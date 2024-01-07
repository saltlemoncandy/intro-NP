import socket

if __name__ == '__main__':
    fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    fd.connect(('inp.zoolab.org', 10301))
    fd.settimeout(0.5)
    
    while True:
        data = fd.recv(1024)
        description = data.decode('utf-8')
        print(description)
        maze_sz_idx = description.find('Note1: Size')
        if (maze_sz_idx == -1):
            print("cannot find index")
            continue
        
        width = int(description[maze_sz_idx+26 : maze_sz_idx+28])
        height = int(description[maze_sz_idx+31 : maze_sz_idx+33])
        
        map = description.find('\n#')
        maze = description[map+1 : map+1+(width+1)*height]        
        maze = maze.split('\n')
        for (w, h) in enumerate(maze):
            for (i, symbol) in enumerate(h):
                if symbol == '*': 
                    start = (w, i)
                elif symbol == 'E':
                    end = (w, i)
        path = ""
        x = start[1] - end[1]
        y = start[0] - end[0]
        if x>0:
            while x>0:
                path += 'A'
                x -= 1
        elif x<0:
            while x<0:
                path += 'D'
                x += 1
        if y>0:
            while y>0:
                path += 'W'
                y -= 1
        elif y<0:
            while y<0:
                path += 'S'
                y += 1
        print("path: ", path)
        path = path + '\n'
        fd.send(path.encode())
        fd.recv(1024)
        flag = fd.recv(1024).decode()[1:]
        print(flag)
        fd.close()
        break  