/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Sergio R. Caprile - clarifications and/or documentation extension
 *
 * Description:
 * Normal topic name is automatically registered at subscription, then
 * a message is published and the node receives it itself
 *******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "MQTTSNPacket.h"
#include "transport.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#define CPU_FREQ 2900

struct quantum {
	unsigned long long last;
	unsigned long long size;
	unsigned long long curr;
};

struct res {
	unsigned long long s;
	unsigned long long l;
};

struct quantum global_limitor;
unsigned long send_rate_limit = 0;
int num_pub_req = 10000;
int publisher = 0;
int duration = 10;

static inline uint64_t
ps_tsc(void)
{
	unsigned long a, d, c;

	__asm__ __volatile__("rdtsc" : "=a" (a), "=d" (d), "=c" (c) : : );

	return ((uint64_t)d << 32) | (uint64_t)a;
}

static void
quantum_init(struct quantum *q, unsigned long limit)
{
	q->curr = q->last = 0;
	if (limit == 0) {
		q->size = limit;
	} else {
		q->size = (unsigned long long)CPU_FREQ * 1000000 / limit;
	}
}

struct opts_struct
{
	char* clientid;
	int   qos;
	char* host;
	int   server_port;
	int   client_port;
};

struct result
{
	int finished;
	unsigned long res_dist[10];
	unsigned long avg;
};

struct opts_struct opts = {(char*)"testClient", 1, "10.10.1.2", 443};
struct result result;

void getopts(int argc, char** argv)
{
	int count = 1;

	while (count < argc) {
		if (strcmp(argv[count], "--host") == 0) {
			if (++count < argc)
				opts.host = argv[count];
		}
		else if (strcmp(argv[count], "--clientid") == 0) {
			if (++count < argc)
				opts.clientid = argv[count];
		}
		else if (strcmp(argv[count], "--server_port") == 0) {
			if (++count < argc)
				opts.server_port = atoi(argv[count]);
		}
		else if (strcmp(argv[count], "--client_port") == 0) {
			if (++count < argc)
				opts.client_port = atoi(argv[count]);
		}
		else if (strcmp(argv[count], "--qos") == 0) {
			if (++count < argc)
				opts.qos = atoi(argv[count]);
		}
		else if (strcmp(argv[count], "--rate") == 0) {
			if (++count < argc)
				send_rate_limit = (unsigned long)atoi(argv[count]);
		}
		else if (strcmp(argv[count], "--duration") == 0) {
			if (++count < argc)
				duration = atoi(argv[count]);
		}
		else if (strcmp(argv[count], "--publisher") == 0) {
			publisher = 1;
		}
		count ++;
	}
}

void print_res(struct res *res)
{
	int i = 0;
	for (i = 0; i < num_pub_req; i++) {
		if (res[i].l != 0)
			printf("%llu\n", res[i].l/CPU_FREQ);
	}
}

void read_publish(int qos, char* host, int port, struct res* res)
{
	unsigned short packet_id;
	int payloadlen;
	unsigned char* payload;
	unsigned char dup, retained;
	MQTTSN_topicid pubtopic;
	unsigned char buf[200];
	int buflen = sizeof(buf);
	int len, rc;

	while (MQTTSNPacket_read(buf, buflen, transport_getdata) == MQTTSN_PUBLISH)
	{
		MQTTSNDeserialize_publish(&dup, &qos, &retained, &packet_id, &pubtopic, &payload, &payloadlen, buf, buflen);
		rc = MQTTSNDeserialize_publish(&dup, &qos, &retained, &packet_id, &pubtopic,
				&payload, &payloadlen, buf, buflen);
		assert(rc == 1);

		res[packet_id].l = ps_tsc()-res[packet_id].s;

		if (qos == 1)
		{
			len = MQTTSNSerialize_puback(buf, buflen, pubtopic.data.id, packet_id, MQTTSN_RC_ACCEPTED);
			rc = transport_sendPacketBuffer(host, port, buf, len);
			assert(rc!=0);
		}
	}
	return;
}

int done = 0;

void handle(int signum) {
	done = 1;
}

int main(int argc, char** argv)
{
	int rc = 0;
	int mysock;
	unsigned char buf[200];
	int buflen = sizeof(buf);
	MQTTSN_topicid topic;
	struct sockaddr_in clientaddr;

	unsigned char* payload = (unsigned char*)"XXXXXXXXXXXXXXXX";
	int payloadlen = strlen((char*)payload);
	int len = 0;
	unsigned char dup = 0;
	//int qos = 1;
	unsigned char retained = 0;
	short packetid = 1;
	char *topicname = "a long topic name";
	char filename[100];
	//char *host = "127.0.0.1";
	//int port = 1883;
	MQTTSNPacket_connectData options = MQTTSNPacket_connectData_initializer;
	unsigned short topicid;
	int cnt = 0;
	unsigned long long tstart;
	//unsigned long long start = 0, end;

	if (argc > 1)
		getopts(argc, argv);

	int port = opts.server_port;
	char *host = opts.host;
	int qos = opts.qos;
	//unsigned long long *res_array;
	//res_array = malloc(sizeof(unsigned long long) * num_pub_req);
	struct res *res = malloc(sizeof(struct res) * num_pub_req);
	memset(res, 0, sizeof(struct res) * num_pub_req);

	signal(SIGALRM, handle);

	mysock = transport_open();
	if(mysock < 0) {
		perror("socket allocate failed");
		exit(-1);
	}

	memset(&clientaddr, 0, sizeof(struct sockaddr_in));

	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = inet_addr("10.10.1.1");
	clientaddr.sin_port = htons(opts.client_port);

	sprintf(filename, "./logs/res_%d", opts.client_port);
	printf("port: %d\n", opts.client_port);

	if ((rc = bind(mysock, (struct sockaddr *)&clientaddr, sizeof(struct sockaddr_in))) < 0) {
		perror("bind");
	}

	//printf("Sending to hostname %s port %d\n", host, port);

	//options.clientID.cstring = "pub0sub1 MQTT-SN";
	options.clientID.cstring = opts.clientid;

	len = MQTTSNSerialize_connect(buf, buflen, &options);
	rc = transport_sendPacketBuffer(host, port, buf, len);

	/* wait for connack */
	if (MQTTSNPacket_read(buf, buflen, transport_getdata) == MQTTSN_CONNACK)
	{
		int connack_rc = -1;

		if (MQTTSNDeserialize_connack(&connack_rc, buf, buflen) != 1 || connack_rc != 0)
		{
			//printf("Unable to connect, return code %d\n", connack_rc);
			assert(0);
		}
	}
	else
		assert(0);

	/* subscribe */
	topic.type = MQTTSN_TOPIC_TYPE_NORMAL;
	topic.data.long_.name = topicname;
	topic.data.long_.len = strlen(topic.data.long_.name);
	len = MQTTSNSerialize_subscribe(buf, buflen, 0, 2, packetid, &topic);
	rc = transport_sendPacketBuffer(host, port, buf, len);

	if (MQTTSNPacket_read(buf, buflen, transport_getdata) == MQTTSN_SUBACK) 	/* wait for suback */
	{
		unsigned short submsgid;
		int granted_qos;
		unsigned char returncode;

		rc = MQTTSNDeserialize_suback(&granted_qos, &topicid, &submsgid, &returncode, buf, buflen);
		if (granted_qos != 2 || returncode != 0)
		{
			assert(0);
			printf("granted qos != 2, %d return code %d\n", granted_qos, returncode);
			goto exit;
		}
		else
			printf("suback topic id %d\n", topicid);
	}
	else
		goto exit;

	//printf("Publishing\n");
	fcntl(mysock, F_SETFL, O_NONBLOCK);
	tstart = ps_tsc();
	quantum_init(&global_limitor, send_rate_limit);
	//quantum_start(&global_limitor);
	//start = ps_tsc();
	/* publish with short name */
	alarm(duration);
	while (!done) {
		if(publisher == 1) {
			if (global_limitor.size > 0) {
				global_limitor.curr = (ps_tsc() - tstart)/global_limitor.size;
				if (global_limitor.last < global_limitor.curr) {
					topic.type = MQTTSN_TOPIC_TYPE_NORMAL;
					topic.data.id = topicid;
					++packetid;
					len = MQTTSNSerialize_publish(buf, buflen, dup, qos, retained, packetid,
							topic, payload, payloadlen);
					rc = transport_sendPacketBuffer(host, port, buf, len);
					assert(rc == 0);
					global_limitor.last++;
					cnt++;
					res[packetid].s = ps_tsc();
				}
			}
		}
		read_publish(qos, host, port, res);
	}
	
exit:
	len = MQTTSNSerialize_disconnect(buf, buflen, 0);
	rc = transport_sendPacketBuffer(host, port, buf, len);

	MQTTSNPacket_read(buf, buflen, transport_getdata);

	transport_close();
	print_res(res);
	printf("SNET: %d\n", cnt);

	return 0;
}
