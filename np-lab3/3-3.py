import socket
        
def move_up(maze, fd, view_height):
    for m in maze:
        if '*' in m:
            semicol = m.find(':')
            start_y = int(m[semicol-2: semicol])
            if (start_y < view_height // 2): 
                return maze, start_y
            
            move = 'I' * (start_y - view_height // 2) + '\n'
            fd.send(move.encode())

            received = fd.recv(1024).decode()
            fd.recv(1024).decode()
            
            return received.split('\n'), start_y
    

def move_left(maze, fd, view_width):
    pre_maze = maze
    while True:
        maze_row = pre_maze[1].split(': ')[1]
        if (maze_row[0] == ' '):
            break
        
        move = 'J' * view_width + '\n'
        fd.send(move.encode())
        received = fd.recv(1024).decode()
        fd.recv(1024).decode()
        pre_maze = received.split('\n')
    
    return pre_maze
                
def mapping(maze, fd, vp_size, maze_size):
    view_height, view_width = vp_size
    mh, mw = maze_size

    check_row = maze[1].split(': ')[1]
    padding_spaces = -1
    for idx, m in enumerate(check_row):
        if (m != ' '):
            padding_spaces = idx
            break
    
    if padding_spaces == -1:
        padding_spaces = view_width
    
    move = 'L' * padding_spaces + '\n'
    fd.send(move.encode())
    received = fd.recv(1024).decode()
    fd.recv(1024).decode()
    pre_maze = received.split('\n')
    
    whole_maze = []
    direction = False
    for _ in range(mh // view_height + 1):
        maze_row = ["" for _ in range(view_height)]
        for i in range(mw // view_width + 1):
            idx = 0
            for row in pre_maze:
                m = row.split(': ')
                
                if len(m) == 2:
                    m = m[1].replace(' ', '')
                    m = m.replace('\n','')
                    if not direction:
                        maze_row[idx] += m
                    else:
                        maze_row[idx] = m + maze_row[idx]
                    idx += 1
                    
            if i != mw // view_width:
                move = 'L' * view_width + '\n' if not direction else 'J' * view_width + '\n'
                fd.send(move.encode())
                received = fd.recv(1024).decode()
                fd.recv(1024).decode()
                pre_maze = received.split('\n')
        
        for mr in maze_row:
            whole_maze.append(mr)
            
        move = 'K' * view_height + '\n'
        fd.send(move.encode())
        received = fd.recv(1024).decode()
        fd.recv(1024).decode()
        pre_maze = received.split('\n')
        direction = not direction
            
    return whole_maze
    
def find_path(maze, start):
    # BFS
    queue = []
    queue.append((start, ""))
    visited = []
    visited.append(start)
    path = ''
    while len(queue):
        cur, cur_path = queue.pop(0)
        y = cur[0]
        x = cur[1]
        if maze[y][x] == 'E':
            path = cur_path
            break
        if maze[y-1][x] in ['.', 'E'] and (y-1, x) not in visited:
            queue.append(((y-1, x), cur_path+'W'))
            visited.append((y-1, x))
        if maze[y+1][x] in ['.', 'E'] and (y+1, x) not in visited:
            queue.append(((y+1, x), cur_path+'S'))
            visited.append((y+1, x))
        if maze[y][x-1] in ['.', 'E'] and (y, x-1) not in visited:
            queue.append(((y, x-1) , cur_path+'A'))
            visited.append((y, x-1))
        if maze[y][x+1] in ['.', 'E'] and (y, x+1) not in visited:
            queue.append(((y, x+1), cur_path+'D'))
            visited.append((y, x+1))
    return path
def send_path(fd, path):
    fd.send(path.encode())    
    flag = ''
    while True:
        received = fd.recv(1024).decode()
        flag += received.replace('\n', '')
        if not received:
            break
    return flag
if __name__ == '__main__':
    fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    fd.connect(('inp.zoolab.org', 10303))
    # fd.settimeout(0.5)
    
    while True:
        received = fd.recv(1024)
        info = received.decode('utf-8')
        print('info\n', info)
        maze_sz_idx = info.find('Note1: Size')
        view_idx = info.find('Note2: View')
        if (maze_sz_idx == -1 or view_idx == -1):
            continue
        
        
        width = int(info[maze_sz_idx+26:maze_sz_idx+29])
        height = int(info[maze_sz_idx+32:maze_sz_idx+35])

        view_width = int(info[view_idx+24:view_idx+26])
        view_height = int(info[view_idx+29:view_idx+31])
        
        info = info[view_idx+31:]
        pre_maze = info.split('\n')
        
        pre_maze, start_y = move_up(pre_maze, fd, view_height)        
        pre_maze = move_left(pre_maze, fd, view_width)                

        
        maze = mapping(pre_maze, fd, (view_height, view_width), (height, width))
        # print('-----whole maze-----\n'+'\n'.join(maze))
        
        start_x = maze[start_y].find('*')
        
        path = find_path(maze, (start_y, start_x))
        flag = send_path(fd, path+'\n')
        print(flag)
        fd.close()
        break  