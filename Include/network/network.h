/*
 * network.h
 *
 *  Created on: Aug 1, 2017
 *      Author: wizath
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "wizchip_conf.h"

#define DHCP_SOCKET				7
#define DHCP_MAX_RETRIES		3

// wiznet library functions
void wizchip_select(void);
void wizchip_deselect(void);
void wizchip_write(uint8_t wb);
uint8_t wizchip_read();

// network configuration
void net_init(void);
void net_conf(wiz_NetInfo * netinfo);
void set_net_conf(uint8_t * ipsrc, uint8_t * ipdst, uint8_t * subnet);
void display_net_conf(void);
void set_mac_address(uint8_t * macaddress);

// server task restart
void prvRestartServerTask(void);

// dhcp related
void ldhcp_ip_assign(void);
void ldhcp_ip_conflict(void);

// dhcp task control
void prvDHCPTask(void *pvParameters);
void prvDHCPTaskStart(void);
void prvDHCPTaskStop(void);
void prvDHCPTaskRestart(void);

#endif /* NETWORK_H_ */
