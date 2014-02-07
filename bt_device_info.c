/*
 *  bt_device_info
 *  Copyright 2014 Simon Wiesmann
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *
 *  Author: Simon Wiesmann
 *  A lot of this code comes from tools/libs of the bluez project
 *  (http://www.bluez.org; especially helpful in the process were
 *  tools/hcitool.c, tools/hcieventmask.c and lib/hci.c)
 */


#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <getopt.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>


#define OPT_NO_OPTION 0
#define OPT_REQUIRED  1
#define OPT_OPTIONAL  2

// program options
static int  opt_verbose     = 0;
static int  opt_unsupported = 0;
static int  opt_color       = 0;

// bash font styles vor colorized output mode
#define STYLE_HEADLINE  "[1;35m"  // bold magenta
#define STYLE_LABEL     "[21;32m" // normal green
#define STYLE_TEXT      "[21;97m" // normal white



static char ESC=27;
void switch_to_style(char* color)
{
    if(opt_color)
        printf("%c%s" , ESC, color);
}


// from bluez v1.13 lib/hci.c
typedef struct {
    char *str;
    unsigned int val;
} hci_map;


void printDeviceFlags(uint32_t dev_flags)
{

    // from bluez v1.13 lib/hci.c
    hci_map dev_flags_map[] = {
        { "UP",      HCI_UP      },
        { "INIT",    HCI_INIT    },
        { "RUNNING", HCI_RUNNING },
        { "RAW",     HCI_RAW     },
        { "PSCAN",   HCI_PSCAN   },
        { "ISCAN",   HCI_ISCAN   },
        { "INQUIRY", HCI_INQUIRY },
        { "AUTH",    HCI_AUTH    },
        { "ENCRYPT", HCI_ENCRYPT },
        { NULL }
    };

    hci_map *map_ptr = dev_flags_map;

    // HCI flags
    switch_to_style(STYLE_LABEL);
    printf("    flags:\t\n");
    switch_to_style(STYLE_TEXT);

    // print all features
    if(opt_unsupported) {
        while (map_ptr->str) {
            printf("        %s:\t", map_ptr->str);
            if(strlen(map_ptr->str) < 7 )
                printf("\t");
            printf("%u\n", (hci_test_bit(map_ptr->val, &dev_flags) > 0));
            map_ptr++;
        }
        return;
    }

    // only print features supported by the adapter
    while (map_ptr->str) {
        if (hci_test_bit(map_ptr->val, &dev_flags))
            printf("        %s\n", map_ptr->str);
        map_ptr++;
    }
}


void printLmpFeatures(uint8_t* lmp_features)
{

    // from bluez v1.13 lib/hci.c
    /* LMP features mapping */
    static hci_map lmp_features_map[8][9] = {
        {	/* Byte 0 */
            { "3-slot packets",	    LMP_3SLOT	},	/* Bit 0 */
            { "5-slot packets",	    LMP_5SLOT	},	/* Bit 1 */
            { "encryption",	        LMP_ENCRYPT	},	/* Bit 2 */
            { "slot offset",	    LMP_SOFFSET	},	/* Bit 3 */
            { "timing accuracy",	LMP_TACCURACY	},	/* Bit 4 */
            { "role switch",	    LMP_RSWITCH	},	/* Bit 5 */
            { "hold mode",	        LMP_HOLD	},	/* Bit 6 */
            { "sniff mode",	        LMP_SNIFF	},	/* Bit 7 */
            { NULL }
        },
        {	/* Byte 1 */
            { "park state",	        LMP_PARK	},	/* Bit 0 */
            { "RSSI",		        LMP_RSSI	},	/* Bit 1 */
            { "channel quality",	LMP_QUALITY	},	/* Bit 2 */
            { "SCO link",		    LMP_SCO		},	/* Bit 3 */
            { "HV2 packets",	    LMP_HV2		},	/* Bit 4 */
            { "HV3 packets",	    LMP_HV3		},	/* Bit 5 */
            { "u-law log",	        LMP_ULAW	},	/* Bit 6 */
            { "A-law log",	        LMP_ALAW	},	/* Bit 7 */
            { NULL }
        },
        {	/* Byte 2 */
            { "CVSD",		        LMP_CVSD	},	/* Bit 0 */
            { "paging scheme",	    LMP_PSCHEME	},	/* Bit 1 */
            { "power control",	    LMP_PCONTROL	},	/* Bit 2 */
            { "transparent SCO",	LMP_TRSP_SCO	},	/* Bit 3 */
            { "broadcast encrypt",  LMP_BCAST_ENC	},	/* Bit 7 */
            { NULL }
        },
        {	/* Byte 3 */
            { "no. 24>",		    0x01		},	/* Bit 0 */
            { "EDR ACL 2 Mbps",	    LMP_EDR_ACL_2M	},	/* Bit 1 */
            { "EDR ACL 3 Mbps",	    LMP_EDR_ACL_3M	},	/* Bit 2 */
            { "enhanced iscan",	    LMP_ENH_ISCAN	},	/* Bit 3 */
            { "interlaced iscan",	LMP_ILACE_ISCAN	},	/* Bit 4 */
            { "interlaced pscan",	LMP_ILACE_PSCAN	},	/* Bit 5 */
            { "inquiry with RSSI",  LMP_RSSI_INQ	},	/* Bit 6 */
            { "extended SCO",	    LMP_ESCO	},	/* Bit 7 */
            { NULL }
        },
        {	/* Byte 4 */
            { "EV4 packets",	    LMP_EV4		},	/* Bit 0 */
            { "EV5 packets",	    LMP_EV5		},	/* Bit 1 */
            { "no. 34",		        0x04		},	/* Bit 2 */
            { "AFH cap. slave",	    LMP_AFH_CAP_SLV	},	/* Bit 3 */
            { "AFH class. slave",	LMP_AFH_CLS_SLV	},	/* Bit 4 */
            { "BR/EDR not supp.",	LMP_NO_BREDR	},	/* Bit 5 */
            { "LE support",	        LMP_LE		},	/* Bit 6 */
            { "3-slot EDR ACL",	    LMP_EDR_3SLOT	},	/* Bit 7 */
            { NULL }
        },
        {	/* Byte 5 */
            { "5-slot EDR ACL",	    LMP_EDR_5SLOT	},	/* Bit 0 */
            { "sniff subrating",	LMP_SNIFF_SUBR	},	/* Bit 1 */
            { "pause encryption",	LMP_PAUSE_ENC	},	/* Bit 2 */
            { "AFH cap. master",	LMP_AFH_CAP_MST	},	/* Bit 3 */
            { "AFH class. master",  LMP_AFH_CLS_MST	},	/* Bit 4 */
            { "EDR eSCO 2 Mbps",	LMP_EDR_ESCO_2M	},	/* Bit 5 */
            { "EDR eSCO 3 Mbps",	LMP_EDR_ESCO_3M	},	/* Bit 6 */
            { "3-slot EDR eSCO",	LMP_EDR_3S_ESCO	},	/* Bit 7 */
            { NULL }
        },
        {	/* Byte 6 */
            { "extended inquiry",	LMP_EXT_INQ	},	/* Bit 0 */
            { "LE and BR/EDR",	    LMP_LE_BREDR	},	/* Bit 1 */
            { "no. 50",		        0x04		},	/* Bit 2 */
            { "simple pairing",	    LMP_SIMPLE_PAIR	},	/* Bit 3 */
            { "encapsulated PDU",	LMP_ENCAPS_PDU	},	/* Bit 4 */
            { "err. data report",	LMP_ERR_DAT_REP	},	/* Bit 5 */
            { "non-flush flag",	    LMP_NFLUSH_PKTS	},	/* Bit 6 */
            { "no. 55",		        0x80		},	/* Bit 7 */
            { NULL }
        },
        {	/* Byte 7 */
            { "LSTO",		        LMP_LSTO	},	/* Bit 1 */
            { "inquiry TX power",	LMP_INQ_TX_PWR	},	/* Bit 1 */
            { "EPC",		        LMP_EPC		},	/* Bit 2 */
            { "no. 59",		        0x08		},	/* Bit 3 */
            { "no. 60",		        0x10		},	/* Bit 4 */
            { "no. 61",		        0x20		},	/* Bit 5 */
            { "no. 62",		        0x40		},	/* Bit 6 */
            { "extended features",  LMP_EXT_FEAT	},	/* Bit 7 */
            { NULL }
        },
    };

    // lmp features
    char *tmp = lmp_featurestostr(lmp_features, "\t", 63);
    switch_to_style(STYLE_LABEL);
    printf("    LMP features:\n");
    switch_to_style(STYLE_TEXT);

    char *str;
    int i;

    // print all features
    if(opt_unsupported) {
        for (i = 0; i < 8; i++) {
            hci_map *map_ptr = lmp_features_map[i];

            while (map_ptr->str) {
                if (map_ptr->val & lmp_features[i]) {
                    printf("        %s:\t", map_ptr->str);
                    if(strlen(map_ptr->str) < 15 )
                        printf("\t");
                    if(strlen(map_ptr->str) < 7 )
                        printf("\t");
                    printf("%u\n", (hci_test_bit(map_ptr->val, &lmp_features[i]) > 0));
                } //endif
                map_ptr++;
            } //endwhile
        } //endfor
        return;
    }


    // only print features supported by the adapter
    for (i = 0; i < 8; i++) {
        hci_map *map_ptr = lmp_features_map[i];

        while (map_ptr->str) {
            if ( (map_ptr->val & lmp_features[i]) &&
                    (hci_test_bit(map_ptr->val, &lmp_features[i]) > 0) ) {
                printf("        %s\n", map_ptr->str);
            } //endif
            map_ptr++;
        } //endwhile
    } //endfor
}





void printPackedTypes(uint32_t pkt_type)
{

    // from bluez v1.13 lib/hci.c
    /* HCI packet type mapping */
    hci_map pkt_type_map[] = {
        { "DM1",   HCI_DM1  },
        { "DM3",   HCI_DM3  },
        { "DM5",   HCI_DM5  },
        { "DH1",   HCI_DH1  },
        { "DH3",   HCI_DH3  },
        { "DH5",   HCI_DH5  },
        { "HV1",   HCI_HV1  },
        { "HV2",   HCI_HV2  },
        { "HV3",   HCI_HV3  },
        { "2-DH1", HCI_2DH1 },
        { "2-DH3", HCI_2DH3 },
        { "2-DH5", HCI_2DH5 },
        { "3-DH1", HCI_3DH1 },
        { "3-DH3", HCI_3DH3 },
        { "3-DH5", HCI_3DH5 },
        { NULL }
    };

    // from bluez v1.13 lib/hci.c
    hci_map sco_ptype_map[] = {
        { "HV1",   0x0001   },
        { "HV2",   0x0002   },
        { "HV3",   0x0004   },
        { "EV3",   HCI_EV3  },
        { "EV4",   HCI_EV4  },
        { "EV5",   HCI_EV5  },
        { "2-EV3", HCI_2EV3 },
        { "2-EV5", HCI_2EV5 },
        { "3-EV3", HCI_3EV3 },
        { "3-EV5", HCI_3EV5 },
        { NULL }
    };


    switch_to_style(STYLE_LABEL);
    printf("    ACL packet types:\n");
    switch_to_style(STYLE_TEXT);

    hci_map* pkt_types = pkt_type_map;
    hci_map* sco_pkt_types = sco_ptype_map;


    // print all packet types
    if(opt_unsupported) {
        // acl packet types
        while (pkt_types->str) {
            printf("        %s\t\t", pkt_types->str);
            printf("%u\n", (hci_test_bit(pkt_types->val, &pkt_type) > 0));
            pkt_types++;
        }

        switch_to_style(STYLE_LABEL);
        printf("    SCO packet types:\n");
        switch_to_style(STYLE_TEXT);

        // synchronous connection packet types
        while (sco_pkt_types->str) {
            printf("        %s\t\t", sco_pkt_types->str);
            printf("%u\n", (hci_test_bit(sco_pkt_types->val, &pkt_type) > 0));
            sco_pkt_types++;
        }

        return;
    }


    // only print packet types supported by the adapter
    // acl packet types
    while (pkt_types->str) {
        if (hci_test_bit(pkt_types->val, &pkt_type))
            printf("        %s\n", pkt_types->str);
        pkt_types++;
    }

    switch_to_style(STYLE_LABEL);
    printf("    SCO packet types:\n");
    switch_to_style(STYLE_TEXT);

    // synchronous connection packet types
    while (sco_pkt_types->str) {
        if (hci_test_bit(sco_pkt_types->val, &pkt_type))
            printf("        %s\n", sco_pkt_types->str);
        sco_pkt_types++;
    }


}


void printLinkPolicy(uint32_t link_policy) {

    // link policy
    switch_to_style(STYLE_LABEL);
    printf("    link_policy:\n");
    switch_to_style(STYLE_TEXT);

    if(opt_unsupported){
        printf("        HCI_LP_RSWITCH:\t%u\n", hci_test_bit(HCI_LP_RSWITCH, &link_policy) > 0);
        printf("        HCI_LP_HOLD:\t%u\n",    hci_test_bit(HCI_LP_HOLD,    &link_policy) > 0);
        printf("        HCI_LP_SNIFF:\t%u\n",   hci_test_bit(HCI_LP_SNIFF,   &link_policy) > 0);
        printf("        HCI_LP_PARK:\t%u\n",    hci_test_bit(HCI_LP_PARK,    &link_policy) > 0);
        return;
    }


    if (hci_test_bit(HCI_LP_RSWITCH, &link_policy))
        printf("        HCI_LP_RSWITCH\n");
    if (hci_test_bit(HCI_LP_HOLD,    &link_policy))
        printf("        HCI_LP_HOLD\n");
    if (hci_test_bit(HCI_LP_SNIFF,   &link_policy))
        printf("        HCI_LP_SNIFF\n");
    if (hci_test_bit(HCI_LP_PARK,    &link_policy))
        printf("        HCI_LP_PARK\n");
}


// TODO (simon): implement unsupported option
void printVerbose(struct hci_dev_info hciDevInfo, struct hci_version hciVersion)
{
    printDeviceFlags(hciDevInfo.flags);

    printLmpFeatures(hciDevInfo.features);

    printPackedTypes(hciDevInfo.pkt_type);

    printLinkPolicy(hciDevInfo.link_policy);

    // link mode
    switch_to_style(STYLE_LABEL);
    printf("    link_mode:\t\t");
    switch_to_style(STYLE_TEXT);
    printf("%s\n", hci_lmtostr(hciDevInfo.link_mode));

    // asynchronous connection-less mtu
    switch_to_style(STYLE_LABEL);
    printf("    acl_mtu:\t\t");
    switch_to_style(STYLE_TEXT);
    printf("%u\n", hciDevInfo.acl_mtu);

    // asynchronous connection-less packets
    switch_to_style(STYLE_LABEL);
    printf("    acl_pkts:\t\t");
    switch_to_style(STYLE_TEXT);
    printf("%u\n", hciDevInfo.acl_pkts);

    // synchronous connection-based mtu
    switch_to_style(STYLE_LABEL);
    printf("    sco_mtu:\t\t");
    switch_to_style(STYLE_TEXT);
    printf("%u\n", hciDevInfo.sco_mtu);

    // synchronous connection-based packets
    switch_to_style(STYLE_LABEL);
    printf("    sco_pkts:\t\t");
    switch_to_style(STYLE_TEXT);
    printf("%u\n", hciDevInfo.sco_pkts);

    // device statistcs
    switch_to_style(STYLE_LABEL);
    printf("    device stats:\t\n");
    switch_to_style(STYLE_TEXT);
    struct hci_dev_stats hciDevStats = hciDevInfo.stat;
    printf("        err_rx:\t\t%u\n", hciDevStats.err_rx);
    printf("        err_tx:\t\t%u\n", hciDevStats.err_tx);
    printf("        cmd_tx:\t\t%u\n", hciDevStats.cmd_tx);
    printf("        evt_rx:\t\t%u\n", hciDevStats.evt_rx);
    printf("        acl_tx:\t\t%u\n", hciDevStats.acl_tx);
    printf("        acl_rx:\t\t%u\n", hciDevStats.acl_rx);
    printf("        sco_tx:\t\t%u\n", hciDevStats.sco_tx);
    printf("        sco_rx:\t\t%u\n", hciDevStats.sco_rx);
    printf("        byte_rx:\t%u\n", hciDevStats.byte_rx);
    printf("        byte_tx:\t%u\n", hciDevStats.byte_tx);
}



int print_each_adapter_info(int socket, int dev_id, long arg)
{

    int hciSocket;
    struct hci_dev_info hciDevInfo;
    struct hci_version  hciVersion;

    // reserve memory for the device info struct
    memset(&hciDevInfo, 0x00, sizeof(hciDevInfo));

    // open HCI socket
    hciSocket = hci_open_dev(dev_id);
    if (hciSocket == -1) {
        fprintf(stderr, "adapterState unsupported on device %d\n",
                dev_id);
        return -1;
    }
    hciDevInfo.dev_id = dev_id;

    if (hci_devinfo(dev_id, &hciDevInfo) < 0) {
        fprintf(stderr, "Can't get device info for hci%d: %s (%d)\n",
                dev_id, strerror(errno), errno);
        hci_close_dev(hciSocket);
        exit(1);
    }

    if (hci_read_local_version(hciSocket, &hciVersion, 1000) < 0) {
        fprintf(stderr, "Can't read version info for hci%d: %s (%d)\n",
                dev_id, strerror(errno), errno);
        hci_close_dev(hciSocket);
        exit(1);
    }

    // Link Management Protocol features
    uint8_t* lmp_features = &hciDevInfo.features;

    switch_to_style(STYLE_HEADLINE);
    printf("\n%s ------------------------------------------- \n",   hciDevInfo.name);

    // device id
    switch_to_style(STYLE_LABEL);
    printf("    device id:\t\t");
    switch_to_style(STYLE_TEXT);
    printf("%u\n", hciDevInfo.dev_id);

    // manufacturer
    char *ver = lmp_vertostr(hciVersion.lmp_ver);
    switch_to_style(STYLE_LABEL);
    printf("    Manufacturer:\t");
    switch_to_style(STYLE_TEXT);
    printf("%s (%d)\n",
           bt_compidtostr(hciVersion.manufacturer),
           hciVersion.manufacturer);

    // lmp version (link management protocol)
    switch_to_style(STYLE_LABEL);
    printf("    LMP version:");
    switch_to_style(STYLE_TEXT);
    printf("\t%s (0x%x) [subver 0x%x]\n",
           ver ? ver : "n/a",
           hciVersion.lmp_ver, hciVersion.lmp_subver);
    if (ver)
        bt_free(ver);

    // device type
    switch_to_style(STYLE_LABEL);
    printf("    device type:\t");
    switch_to_style(STYLE_TEXT);
    if (((hciDevInfo.type & 0x30) >> 4) != HCI_AMP)
        printf("AMP\n");
    else if (((hciDevInfo.type & 0x30) >> 4) != HCI_BREDR)
        printf("BR/EDR\n");
    else
        printf("UNKNOWN\n");

    // BLE
    switch_to_style(STYLE_LABEL);
    printf("    BLE:\t\t");
    switch_to_style(STYLE_TEXT);
    if(hciVersion.lmp_ver >= 0x06) {

        if (lmp_features[6] & LMP_LE_BREDR)
            printf("capable (dual mode)");
        else if (lmp_features[4] & LMP_LE)
            printf("capable (single mode)");
        else
            printf("UNKNOWN MODE");
    } else
        printf("incapable");

    printf("\n");




    // get bluetooth device address
    bdaddr_t* bdaddr = &hciDevInfo.bdaddr;
    switch_to_style(STYLE_LABEL);
    printf("    device address:\t");
    switch_to_style(STYLE_TEXT);
    int i;
    for(i=5; i>1; i--) {
        printf("%02X:", bdaddr->b[i]);
    }
    printf("%02X\n", bdaddr->b[0]);


    if(opt_verbose)
        printVerbose(hciDevInfo, hciVersion);

    printf("\n");

    close(hciSocket);
    return 0;
}


// TODO (simon): add license info
// TODO (simon): add githubrepo url
void show_help(char* program_name)
{
    printf("\nUsage: %s [OPTION]\n"\
           "Prints driver-level information about the "\
           "Bluetooth devices connected to this machine. "\
           "Uses code from the Bluez project (http://www.bluez.org/).\n\n"\

           "Available options:\n\n"\
           "  -v, --verbose              print more details about the adapter\n"\
           "  -u. --unsupported          also show bluetooth featured this adapter does not support\n"\
           "                             or are simply not activated/active at the moment\n"
           "                             (will be marked as not supported; implies --verbose)\n"\
           "  -c, --color                colorized output for improved readability\n"\

           "  -h, --help                 this text\n", program_name);
}



int main(int argc, char* const  argv[])
{
    char* program_name = argv[0];

    const struct option long_options[] = {
        {"verbose",     OPT_NO_OPTION,        0, 'v'},
        {"unsupported", OPT_NO_OPTION,        0, 'u'},
        {"color",       OPT_NO_OPTION,        0, 'c'},
        {"help",        OPT_NO_OPTION,        0, 'h'},
        {0,0,0,0},
    };

    int opt;

    while (1) {
        /* getopt_long stores the option index here. */
        int getopt_long_index = 0;

        opt = getopt_long (argc, argv, "vuch",
                           long_options, &getopt_long_index);

        /* Detect the end of the options. */
        if (opt == -1)
            break;

        switch (opt) {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[getopt_long_index].flag != 0)
                break;
            printf ("option %s", long_options[getopt_long_index].name);
            if (optarg)
                printf (" with arg %s", optarg);
            printf ("\n");
            break;

        case 'v':
            opt_verbose = 1;
            break;

        case 'u':
            opt_unsupported = 1;
            break;

        case 'c':
            opt_color = 1;
            break;

        case 'h':
            show_help(program_name);
            return 0;
            break;

        case '?':
            /* getopt_long already printed an error message. */
            printf("try --help to see all valid options\n");
            return 0;
            break;

        default:
            printf("Unknown option: %u\n", argv[opt]);
            printf("try --help to see all valid options\n");
            return 0;
        }
    }


    switch_to_style(STYLE_TEXT);
    printf("Bluetooth adapter info:\n");

    // loop through all adapters
    hci_for_each_dev(HCI_UP, print_each_adapter_info, 0);

    return 0;
}

