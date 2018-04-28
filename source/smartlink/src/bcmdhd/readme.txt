* About this demo

It demos how to start/stop Broadcom easy setup protocols, including 3rd-party such as airkiss.

Usage: (type setup -h)
# setup -h
-h: show help message
-d: show debug message
-k <v>: set 16-char key for all protocols
-q: enable qqconnect
-p <v>: bitmask of protocols to enable
  0x0001 - cooee
  0x0002 - neeze
  0x0004 - akiss

To start cooee only:
# setup -p 1

To start qqconnect+akiss:
# setup -p 5 -q

A sample output:
# setup -p 1
state: 0 --> 1
state: 1 --> 2
state: 2 --> 3
state: 3 --> 5
ssid: Broadcom2g
password: 12345678
sender ip: 192.168.43.149
sender port: 0
security: wpa2

* About API
You can enable various protocols with following APIs before calling easy_setup_start():
easy_setup_enable_cooee();
easy_setup_enable_neeze();
easy_setup_enable_akiss();
easy_setup_enable_qqcon();

or just with one call:
easy_setup_enable_protocols(uint16 proto_mask);

where proto_mask is bitmask of following:
  0x0001 - cooee
  0x0002 - neeze
  0x0004 - akiss

For example, to start qqconnect+akiss:
easy_setup_enable_qqcon();
easy_setup_enable_akiss();
easy_setup_start();
/* get ssid/password */
// easy_setup_get_xxx();
easy_setup_stop();

if needed, you can set decryption key for various protocols:
cooee_set_key("0123456789abcdef"); /* cooee_set_key() also works for qqconnect */
akiss_set_key("fedcba9876543210"); /* set akiss decryption key */

With received ssid/password, it's time to feed it to wpa_supplicant:

#!/system/bin/sh

id=$(wpa_cli -i wlan0 add_network)
wpa_cli -i wlan0 set_network $id ssid \"Broadcom2g\"
wpa_cli -i wlan0 set_network $id psk \"12345678\"
wpa_cli -i wlan0 set_network $id key_mgmt WPA-PSK
wpa_cli -i wlan0 set_network $id priority 0
wpa_cli -i wlan0 enable_network $id
wpa_cli -i wlan0 save_config

or you can write one entry to wpa_supplicant.conf and reload it:
network={
    ssid="Broadcom2g"
    psk="12345678"
    key_mgmt=WPA-PSK
}

