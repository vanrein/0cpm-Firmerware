/* Temporary settings during development
 *
 * This file is part of 0cpm Firmerware.
 *
 * 0cpm Firmerware is Copyright (c)2011 Rick van Rein, OpenFortress.
 *
 * 0cpm Firmerware is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 3.
 *
 * 0cpm Firmerware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0cpm Firmerware.  If not, see <http://www.gnu.org/licenses/>.
 */


// TODO: The following nethandlers have not been implemented yet
#include <stdlib.h>
inline uint8_t *net_rtp (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) { return NULL; }
inline uint8_t *net_rtcp (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) { return NULL; }
inline uint8_t *net_sip (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) { return NULL; }
inline uint8_t *net_mdns_resp_error (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) { return NULL; }
inline uint8_t *net_mdns_resp_dyn (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) { return NULL; }
inline uint8_t *net_mdns_resp_std (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) { return NULL; }
inline uint8_t *net_mdns_query_error (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) { return NULL; }
inline uint8_t *net_mdns_query_ok (uint8_t *pkt, uint32_t pktlen, intptr_t *mem) { return NULL; }

