# DNS Tools for Name.com

DNS tools for name.com is a DNS record management utility (the _namecom_dns_ command) and a dynamic DNS client (the _namecom_dyndns_ command).

----------

## Donate, if you found this tool useful

The development of these utilities took numerous hours of development and testing.  If you found that they
are useful to you, then please consider making a [donation of bitcoin.](https://bitpay.com/cart/add?itemId=MkFQg3nj178sM7B2xJdjCf)

All donations help cover maintenance costs.

## Building from Source
1. Clone the source from: ```git clone git@bitbucket.org:manvscode/name.com-dns-tools.git```
2. Build the source: ```make```

## DNS Record Management

The _namecom_dns_ utility allows the management of DNS records for domains registered at https://name.com/.

**Features at a glance**

- The ability to add or update A, CNAME, MX, NS, and TXT records.
- The ability to remove existing DNS records.
- The ability to list out all of the existing DNS records for a domain registered at https://name.com.

### Request API token from Name.com

You can request an API token from https://www.name.com/reseller

### Environment Setup
To use the _namecom_dns_ utility, you will need to setup the _NAMECOM_USERNAME_ and _NAMECOM_API_TOKEN_ environment variables.

On Linux and Mac OS X, if your username is dummyuser, then you can do this like this:
```
export NAMECOM_USERNAME="dummyuser"
export NAMECOM_API_TOKEN="hfS$sdk34lHMkj43nml2jn*71!hqokmGhdsfokmOqMsKSAhqOK98FMMASOOIQM"
```
Make sure to set the correct API token that name.com assigned to your account.

### List out all DNS records for domain
To list out all records for example.com, you can use the -l or --list command line argument like this:
```
$ namecom_dns --list example.com
  _ __   __ _ _ __ ___   ___    ___ ___  _ __ ___
 | '_ \ / _` | '_ ` _ \ / _ \  / __/ _ \| '_ ` _ \
 | | | | (_| | | | | | |  __/ | (_| (_) | | | | | |
 |_| |_|\__,_|_| |_| |_|\___|(_)___\___/|_| |_| |_|
                DNS Record Manager

+-----------------------------------------------------------------------------+
|  Type  Host                 Answer                 TTL  Created             |
+-----------------------------------------------------------------------------+
|     A  xen.example.com      10.15.70.173            60  2017-03-09 14:58:13 |
| CNAME  abc.example.com      example.com            300  2017-02-15 21:34:55 |
|     A  test.example.com     10.15.65.191           300  2017-08-24 10:54:58 |
|     A  example.com          12.34.56.78            300  2015-07-23 23:15:59 |
|    MX  example.com          mail.example.com       300  2010-11-03 16:35:15 |
| CNAME  www.example.com      example.com            300  2010-11-01 11:59:43 |
+-----------------------------------------------------------------------------+
```

### Add or update a DNS record
To add or update a record, you can us the -s or --set command line argument like this:
```
$ namecom_dns --set A pizza.example.com 10.0.0.1 300
  _ __   __ _ _ __ ___   ___    ___ ___  _ __ ___
 | '_ \ / _` | '_ ` _ \ / _ \  / __/ _ \| '_ ` _ \
 | | | | (_| | | | | | |  __/ | (_| (_) | | | | | |
 |_| |_|\__,_|_| |_| |_|\___|(_)___\___/|_| |_| |_|
                DNS Record Manager

Record created.
```
### Remove a DNS record
To remove a record, you can use the -d or --delete command line argument like this:

```
$ namecom_dns --delete pizza.example.com
  _ __   __ _ _ __ ___   ___    ___ ___  _ __ ___
 | '_ \ / _` | '_ ` _ \ / _ \  / __/ _ \| '_ ` _ \
 | | | | (_| | | | | | |  __/ | (_| (_) | | | | | |
 |_| |_|\__,_|_| |_| |_|\___|(_)___\___/|_| |_| |_|
                DNS Record Manager

Record deleted.
```

----------

## Dynamic DNS Client

The _namecom_dyndns_ utility allows for dynamic DNS address records (i.e A records) to be created or updated.

### Update DNS record using public IP address
```
$ namecom_dyndns --host dynamic.example.com
  _ __   __ _ _ __ ___   ___    ___ ___  _ __ ___
 | '_ \ / _` | '_ ` _ \ / _ \  / __/ _ \| '_ ` _ \
 | | | | (_| | | | | | |  __/ | (_| (_) | | | | | |
 |_| |_|\__,_|_| |_| |_|\___|(_)___\___/|_| |_| |_|
               Dynamic DNS Updater

Record updated.
```
### Update DNS record using specific IP address
```
$ namecom_dyndns --host test.unitserver.com --ip-address 10.0.0.1
  _ __   __ _ _ __ ___   ___    ___ ___  _ __ ___
 | '_ \ / _` | '_ ` _ \ / _ \  / __/ _ \| '_ ` _ \
 | | | | (_| | | | | | |  __/ | (_| (_) | | | | | |
 |_| |_|\__,_|_| |_| |_|\___|(_)___\___/|_| |_| |_|
               Dynamic DNS Updater

Record updated.
```

----------



License
-------------
This software is released under an MIT license so you may freely use it for commercial purposes.  However,
please consider making a [donation of Bitcoin](https://bitpay.com/cart/add?itemId=MkFQg3nj178sM7B2xJdjCf)
if you do find it useful.

> Copyright (C) 2016 by Joseph A. Marrero. http://www.joemarrero.com/
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
> 
> The above copyright notice and this permission notice shall be included in
> all copies or substantial portions of the Software.
>   
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
> THE SOFTWARE.

