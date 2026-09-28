#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>

#include "internal.h"

#define ARP_PACKET_BYTES 28u
#define ARP_HTYPE_ETHERNET 1u
#define ARP_PTYPE_IPV4 0x0800u
#define ARP_OP_REQUEST 1u
#define ARP_OP_REPLY 2u
#define ARP_CACHE_SIZE 8u

struct arp_entry {
    uint32_t address;
    uint8_t mac[6];
    uint32_t age;
    int valid;
};

static struct arp_entry cache[ARP_CACHE_SIZE];
static uint32_t age_counter;

static void cache_store(uint32_t address, const uint8_t mac[6])
{
    unsigned index;
    unsigned victim = 0u;
    uint32_t oldest = UINT32_MAX;

    if (address == 0u || mac == 0) return;

    for (index = 0u; index < ARP_CACHE_SIZE; ++index) {
        if (cache[index].valid && cache[index].address == address) {
            net_copy(cache[index].mac, mac, 6u);
            cache[index].age = ++age_counter;
            return;
        }
        if (!cache[index].valid) {
            victim = index;
            oldest = 0u;
            break;
        }
        if (cache[index].age < oldest) {
            oldest = cache[index].age;
            victim = index;
        }
    }

    cache[victim].address = address;
    net_copy(cache[victim].mac, mac, 6u);
    cache[victim].age = ++age_counter;
    cache[victim].valid = 1;
}

static int cache_lookup(uint32_t address, uint8_t mac_out[6])
{
    unsigned index;
    for (index = 0u; index < ARP_CACHE_SIZE; ++index) {
        if (cache[index].valid && cache[index].address == address) {
            net_copy(mac_out, cache[index].mac, 6u);
            cache[index].age = ++age_counter;
            return 1;
        }
    }
    return 0;
}

static int send_request(uint32_t target)
{
    uint8_t packet[ARP_PACKET_BYTES];
    static const uint8_t broadcast[6] = {
        0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu
    };
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();

    if (config == 0) return 0;
    net_clear(packet, sizeof(packet));
    net_write_be16(&packet[0], ARP_HTYPE_ETHERNET);
    net_write_be16(&packet[2], ARP_PTYPE_IPV4);
    packet[4] = 6u;
    packet[5] = 4u;
    net_write_be16(&packet[6], ARP_OP_REQUEST);
    net_copy(&packet[8], config->mac, 6u);
    net_write_be32(&packet[14], config->address);
    net_write_be32(&packet[24], target);

    if (!ethernet_send(broadcast, ETHERTYPE_ARP, packet, sizeof(packet))) return 0;
    ++stats->arp_requests;
    return 1;
}

static int send_reply(const uint8_t destination_mac[6], uint32_t destination_ip)
{
    uint8_t packet[ARP_PACKET_BYTES];
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();

    if (config == 0 || destination_mac == 0) return 0;
    net_clear(packet, sizeof(packet));
    net_write_be16(&packet[0], ARP_HTYPE_ETHERNET);
    net_write_be16(&packet[2], ARP_PTYPE_IPV4);
    packet[4] = 6u;
    packet[5] = 4u;
    net_write_be16(&packet[6], ARP_OP_REPLY);
    net_copy(&packet[8], config->mac, 6u);
    net_write_be32(&packet[14], config->address);
    net_copy(&packet[18], destination_mac, 6u);
    net_write_be32(&packet[24], destination_ip);

    if (!ethernet_send(destination_mac, ETHERTYPE_ARP, packet, sizeof(packet))) return 0;
    ++stats->arp_replies;
    return 1;
}

void arp_handle(const uint8_t *packet, size_t length)
{
    const struct net_config *config = net_config_internal();
    uint16_t operation;
    uint32_t sender_ip;
    uint32_t target_ip;

    if (packet == 0 || config == 0 || length < ARP_PACKET_BYTES) return;
    if (net_read_be16(&packet[0]) != ARP_HTYPE_ETHERNET ||
        net_read_be16(&packet[2]) != ARP_PTYPE_IPV4 ||
        packet[4] != 6u || packet[5] != 4u) {
        return;
    }

    operation = net_read_be16(&packet[6]);
    sender_ip = net_read_be32(&packet[14]);
    target_ip = net_read_be32(&packet[24]);

    cache_store(sender_ip, &packet[8]);

    if (operation == ARP_OP_REQUEST && target_ip == config->address) {
        (void)send_reply(&packet[8], sender_ip);
    }
}

int arp_resolve(uint32_t address, uint8_t mac_out[6])
{
    const struct net_config *config = net_config_internal();
    uint32_t next_hop;
    unsigned attempt;

    if (config == 0 || mac_out == 0) return -AXIOM_EINVAL;

    next_hop = ((address & config->netmask) == (config->address & config->netmask)) ?
        address : config->gateway;

    if (cache_lookup(next_hop, mac_out)) return 0;

    for (attempt = 0u; attempt < NET_ARP_ATTEMPTS; ++attempt) {
        uint32_t poll;
        if (!send_request(next_hop)) return -AXIOM_EIO;

        for (poll = 0u; poll < NET_POLL_BUDGET / 8u; ++poll) {
            (void)net_poll(8u);
            if (cache_lookup(next_hop, mac_out)) return 0;
            __asm__ volatile ("pause");
        }
    }
    return -AXIOM_EHOSTUNREACH;
}
