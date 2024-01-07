import socket
import subprocess
import threading
import time
import dpkt
import os



def exec_tcpdump():
    # run tcpdump in subprocess for 3 seconds
    timeout = 3
    tcpdump = subprocess.Popen(['sudo', 'tcpdump', '-ni', 'any', '-Xxnv', 'udp', 'and', 'port', '10495', '-w', 'demo.pcap'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        tcpdump.wait(timeout)
    except subprocess.TimeoutExpired:
        print('finish')
        tcpdump.kill()
        
def parse_packet():
    with open('./demo.pcap', 'rb') as f:
        pcap = dpkt.pcap.Reader(f)

        # get datalink
        print(f"datalink: {pcap.datalink()}") 
        udp_dict = {}
        for ts, buf in pcap:
            sll2 = dpkt.sll2.SLL2(buf)
            ip = sll2.data
            udp = ip.data
            payload = str(udp.data)
            
            # get payload and ip option length (if any)
            payload_len = len(udp.data)
            ip_opt_len = len(ip.opts)

            idx = payload.find("SEQ")
            if idx != -1:
                try:
                    seq_no = int(payload[idx+5:idx+9])
                except:
                    print(seq_no)
                    continue
                udp_dict[seq_no] = [payload, payload_len + ip_opt_len]
        # sort udp_dict by keys
        sorted_udp_dict = sorted(udp_dict.items(), key=lambda x:x[0])

        begin_idx = -1
        end_idx = -1
        for idx, buf in enumerate(sorted_udp_dict):
            if 'BEGIN FLAG' in str(buf[1][0]):
                begin_idx = idx
            elif 'END FLAG' in str(buf[1][0]):
                end_idx = idx
                break
        print(f'key length: {sorted_udp_dict[end_idx][0] - sorted_udp_dict[begin_idx][0] - 1}')
        
        # check packet miss 
        prev_seq_no = sorted_udp_dict[begin_idx][0]
        flag = [''] * 55
        miss_idx = []
        packet_miss = False
        for i in range(begin_idx+1, end_idx):
            flag_no = sorted_udp_dict[i][0] - sorted_udp_dict[begin_idx][0] - 1
            cur_seq_no = sorted_udp_dict[i][0]
            if cur_seq_no - prev_seq_no > 1:
                packet_miss = True
                while prev_seq_no != cur_seq_no:
                    miss_idx.append(prev_seq_no - sorted_udp_dict[begin_idx][0])
                    prev_seq_no += 1
                    print(f'Missing packet {prev_seq_no}')
            else:
                prev_seq_no = cur_seq_no
                flag[flag_no] = chr(sorted_udp_dict[i][1][1])
        if not packet_miss:
            print(' ')
        else:
            print(f'Missed seq no {miss_idx}')
        return flag, miss_idx

def delete_pcap_file():
    try:
        os.remove('./demo.pcap')
        print('demo.pcap file deleted.')
    except FileNotFoundError:
        print('Demo.pcap file not found.')

    
        
if __name__ == '__main__':
    
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    host = "127.0.0.1" 
    port = 10495
    userid = input("enter your id : ")     
    
    s.connect((host, port))
    s.settimeout(0.5)
    
    # send hello
    data = ''
    while data.startswith('OK') == False:
        s.send(f'hello {userid}'.encode())
        try:
            data = s.recv(1024).decode()
            print(data)
        except:
            pass
        if data.startswith('OK'):
            break
    key = data.replace('OK ', '')
    print(f'Key: {key}')
    

    print('recording') 
    # time.sleep(2)  
    f = open('./demo.pcap', 'w')
    f.close()    
    t = threading.Thread(target=exec_tcpdump, daemon=True)
    t.start()
    time.sleep(2)    
    data = ''
    while data == '':
        # print('Sending chals')        
        s.send(f'chals {key}'.encode())
        try:
            data = s.recv(1024).decode()
        except:
            pass    
    while True:
        try:
            data = s.recv(1024) # read socket buffer
        except:
            break        
    t.join()

    
    # parse packet
    guess_flag, guess_idx = parse_packet()
    flag = ''.join(guess_flag) 
    print(f'Flag: {flag}')
    
    # verify flag
    data = ''
    while True:
        s.send(f'verfy {flag}'.encode())
        try:               
            data = s.recv(1024).decode()
        except:
            pass
        if data != '':
            break
    print(data)
    delete_pcap_file()