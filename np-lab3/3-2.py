import socket

if __name__ == '__main__':
    fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    fd.connect(('inp.zoolab.org', 10302))
    fd.settimeout(0.5)
    
    while True:
        data = fd.recv(1024)
        description = data.decode('utf-8')
        print(description)
        maze_sz_idx = description.find('maze =')
        if (maze_sz_idx == -1):
            print("cannot find index")
            continue
        
        width = int(description[maze_sz_idx+7 : maze_sz_idx+9])
        height = int(description[maze_sz_idx+12 : maze_sz_idx+14])
        
        map = description.find('\n#')
        maze = description[map+1 :]    
        while True:
            remained_mazes = fd.recv(1024).decode()
            maze += remained_mazes
            if 'Enter your move(s)>' in remained_mazes:
                break
        print(maze)    
        maze = maze.split('\n')
        maze = maze[:-3]
        for (w, h) in enumerate(maze):
            for (i, symbol) in enumerate(h):
                if symbol == '*': 
                    start = (w, i)
                elif symbol == 'E':
                    end = (w, i)
        print(start)
        print(end)
        # BFS
        q = []
        q.append((start, ""))
        print(q)
        visited = []
        visited.append(start)
        path = ''
        while len(q):
            current,  cur_path = q.pop(0)
            x = current[1]
            y = current[0]
            if maze[y][x] == 'E':
                path = cur_path
                break
            if maze[y+1][x] in ['.', 'E'] and (y+1, x) not in visited:
                q.append(((y+1, x), cur_path+'S'))
                visited.append((y+1, x))
            if maze[y-1][x] in ['.', 'E'] and (y-1, x) not in visited:
                q.append(((y-1, x), cur_path+'W'))
                visited.append((y-1, x))
            if maze[y][x+1] in ['.', 'E'] and (y, x+1) not in visited:
                q.append(((y, x+1), cur_path+'D'))
                visited.append((y, x+1))
            if maze[y][x-1] in ['.', 'E'] and (y, x-1) not in visited:
                q.append(((y, x-1) , cur_path+'A'))
                visited.append((y, x-1))
            
        print("path: ", path)
        path = path + '\n'
        fd.send(path.encode())
        fd.recv(1024)
        flag = fd.recv(1024).decode()[1:]
        print(flag)
        fd.close()
        break  