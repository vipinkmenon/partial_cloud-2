
struct pcap_timeval {
    bpf_int32 tv_sec;       /* seconds */
    bpf_int32 tv_usec;      /* microseconds */
};


struct pcap_sf_pkthdr {
    struct pcap_timeval ts; /* time stamp */
    bpf_u_int32 caplen;     /* length of portion present */
    bpf_u_int32 len;        /* length this packet (off wire) */
};

/* default snap length (maximum bytes per packet to capture) */
#define SNAP_LEN 1518

/* ethernet headers are always exactly 14 bytes [1] */
#define SIZE_ETHERNET 14

/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN 6
#define ETHER_MAX_LEN       1514        /* maximum frame length, excluding CRC */
#define PKT_PAD             0x00        /* packet padding */
#define PCAP_HDR_LEN        16          /* pcap generic header length */
#define PCAP_MAGIC          0xa1b2c3d4  /* pcap magic number */


/* Ethernet header */
struct sniff_ethernet {
        u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
        u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
        u_short ether_type;                     /* IP? ARP? RARP? etc */
};

/* IP header */
struct sniff_ip {
        u_char  ip_vhl;                 /* version << 4 | header length >> 2 */
        u_char  ip_tos;                 /* type of service */
        u_short ip_len;                 /* total length */
        u_short ip_id;                  /* identification */
        u_short ip_off;                 /* fragment offset field */
        #define IP_RF 0x8000            /* reserved fragment flag */
        #define IP_DF 0x4000            /* dont fragment flag */
        #define IP_MF 0x2000            /* more fragments flag */
        #define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
        u_char  ip_ttl;                 /* time to live */
        u_char  ip_p;                   /* protocol */
        u_short ip_sum;                 /* checksum */
        struct  in_addr ip_src,ip_dst;  /* source and dest address */
};
#define IP_HL(ip)               (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)                (((ip)->ip_vhl) >> 4)

void get_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);
void get_config_data(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);
void send_packets(char *device, char *trace_file);
void * recv_loop(void * process_id);
void * process_loop();
int config_fpga(char * partial_file);
int process_data(char * data_file, char * output_file);
