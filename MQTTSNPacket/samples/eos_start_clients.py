import subprocess
import argparse
import time

cur_core = 0
nb_cores = 1
server = "127.0.0.1"
clientid = "kkkkkkkkkk000000"
server_port = "1885"

class client(object):
    def __init__(self, core, client_port, rate, pubnum):
	self.client_port = client_port

        self.args = ["taskset"]
        self.args.extend(["-c", str(core)])
        self.args.extend(["./pub0sub1"])
        self.args.extend(["--host", server])
        self.args.extend(["--clientid", clientid])
        self.args.extend(["--server_port", server_port])
        self.args.extend(["--client_port", str(client_port)])
        self.args.extend(["--qos", "1"])
        self.args.extend(["--rate", str(rate)])
        self.args.extend(["--pubnum", str(pubnum)])
        if (client_port % 2 == 1):
            self.args.extend(["--listener"])

        self.log_file_path, log_file = self.create_log()
        print self.args
        
        self.p = subprocess.Popen(self.args, stdout=log_file)
        log_file.close()

    def stop(self):
        while self.process.poll() is None:
            time.sleep(1)
        self.log_file_path.close()

    def create_log(self):
        file_path = "logs/mqtt_{}".format(self.client_port)
        f = open(file_path, "w+")
        f.write("ab arguments: {}\n\nSTDOUT:\n".format(str(self.args)))
        f.flush()
        return file_path, f
"""
    def parse_result(self):
        with open(self.log_file_path, "r") as f:

            for line in f:
                if line.startswith("Requests per second:"):
                    print line
                    self.request_per_sec = line.split()[3].strip();
                    print self.request_per_sec
                    break

        return self.request_per_sec

def form_ab(
        k = True, 
        nb_requests = 1,
        core = 0
        ):
    args = ["taskset"]
    args.extend(["-c", str(core)])
    args.extend(["ab"])
    if k:
        args.extend(["-k"])
    args.extend(["-n", str(nb_requests)])
    print args
    return args
"""

def increment_core_num(core):
    global cur_core
    p = core + cur_core
    cur_core = (cur_core + 1) % nb_cores
    return p

def start_clients(pubnum, client_port, rate):
    client_list=[]
    for _ in range(pubnum):
        nc = increment_core_num(0)
        client_list.append(client(nc, client_port, rate, pubnum))
        """
        if https_port > 0:
            https_port += 1
        """
        if client_port > 0:
            client_port += 1
    return client_list

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--client_port", help="start of the client port number", type=int)
    parser.add_argument("--rate", help="rate limit", type=int)
    parser.add_argument("--pubnum", help="number of publishes to make", type=int)
    parser.add_argument("--nb_cores", help="number of cores", type=int)
    return parser.parse_args()

if __name__ == '__main__':

    args = parse_args()
    #server = "10.10.1.2"
    #client_port = 11211
    client_port = args.client_port if args.client_port else 11211
    rate = args.rate if args.rate else 500
    pubnum = args.pubnum if args.pubnum else 1
    nb_cores = args.nb_cores if args.nb_cores else 1

    client_list = start_clients(pubnum, client_port, rate)

    #time.sleep(60)
    #total_result = 0

    #for client in client_list:
     #   total_result += float(client.parse_result())

    #print total_result
