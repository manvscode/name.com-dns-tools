/*
 * Copyright (C) 2016 by Joseph A. Marrero. http://www.joemarrero.com/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef _NAMECOM_API_H_
#define _NAMECOM_API_H_

#include <stdbool.h>

#define NAMECOM_API_VERSION  "1.0"

struct namecom_api;
typedef struct namecom_api namecom_api_t;

#ifndef NAMECOM_API_VERBOSE
#define NAMECOM_API_VERBOSE 0L
#endif

namecom_api_t* namecom_api_create        ( const char* username, const char* api_token, bool is_dev, bool verbose );
void           namecom_api_destroy       ( namecom_api_t* api );
const char*    namecom_api_username      ( const namecom_api_t* api );
const char*    namecom_api_token         ( const namecom_api_t* api );
const char*    namecom_api_server        ( const namecom_api_t* api );
const char*    namecom_api_session_token ( const namecom_api_t* api );


bool           namecom_api_login            ( namecom_api_t* api );
bool           namecom_api_logout           ( namecom_api_t* api );
bool           namecom_api_hello            ( namecom_api_t* api );
bool           namecom_api_domains_list     ( namecom_api_t* api );



typedef struct namecom_api_dns_record {
	long id;
	const char* fqdn;
	const char* type;
	const char* content;
	int ttl;
	const char* create_date;
} namecom_api_dns_record_t;

namecom_api_dns_record_t* namecom_api_dns_record_create( long id, const char* fqdn, const char* type, const char* content, int ttl, const char* create_date );
void namecom_api_dns_record_destroy( namecom_api_dns_record_t* dns_record );


namecom_api_dns_record_t** namecom_api_dns_record_list   ( namecom_api_t* api, const char* domain );
bool                       namecom_api_dns_record_add    ( namecom_api_t* api, const char* domain, const char* hostname, const char* type, const char* content, int ttl, int priority, long* id );
bool                       namecom_api_dns_record_remove ( namecom_api_t* api, const char* domain, long id );

#endif /* _NAMECOM_API_H_ */
