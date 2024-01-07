import socket

def send_move(move, s):
    move = move + '\n'
    s.send(move.encode())
    raw = ''
    while True:
        received = s.recv(1024).decode()
        raw += received
        if 'Enter your move(s)>' in raw:
            break
        
        
    # print(f'Move: {move}')
    raw = raw.split('\n')
    
    new_maze = []
    
    for maze_row in raw:
        m = maze_row.split(': ')
        if len(m) == 2:
            row_idx = int(m[0])
            m = m[1].replace('\n', '')
            no_spaces = m.replace(' ', '')
            
            if no_spaces != '':
                new_maze.append((row_idx, m))
    return new_maze
            
def get_flag(move, s):
    move = move + '\n' 
    s.send(move.encode())
    
    flag = ''
    while True:
        received = s.recv(1024).decode()
        flag += received.replace('\n', '')
        if not received:
            break
    return flag

def get_reverse_path(path: str, parent: str):
    to_parent_path = path[len(parent)-1:]
    reverse_path = ""
    for command in to_parent_path[::-1]:
        if command == 'W':
            reverse_path += 'S'
        if command == 'S':
            reverse_path += 'W'
        if command == 'D':
            reverse_path += 'A'
        if command == 'A':
            reverse_path += 'D'
    return reverse_path

def find_path(maze, size, vp, s):
    mh, mw = size
    vph, vpw = vp
    PAD = '_'
    # saving maze to a array
    init_xlen = -1
    for maze_row in maze:
        m = maze_row.split(': ')
        if len(m) == 2:
            m = m[1].replace(' ', '')
            if m != '':
                init_xlen = len(m.replace('\n', ''))
                break

    known_maze = [PAD * init_xlen for _ in range(mh)]
    curx, cury = -1, -1 # the index of * in known_maze
    for maze_row in maze:
        m = maze_row.split(': ')
        if len(m) == 2:
            row_idx = int(m[0])
            m = m[1].replace(' ', '')
            m = m.replace('\n', '')
            if m != '':
                known_maze[row_idx] = m
            
            if '*' in m:
                cury = row_idx
                curx = m.find('*')

    q = []
    visit = []
    flag = ''
    
    q.append([cury, curx, ''])
    visit.append([cury, curx])
    
    while len(q):
        y, x, path = q.pop(-1)
        dead_end = True
        
        if known_maze[y][x] == 'E':
            move = path[-1]
            flag = get_flag(move, s)
            break
            
        # pushing available moves into queue
        if known_maze[y-1][x] in ['.', 'E'] and [y-1, x] not in visit:
            visit.append([y-1, x])
            q.append([y-1, x, path+'W'])
            dead_end = False
            
        if known_maze[y+1][x] in ['.', 'E'] and [y+1, x] not in visit:
            visit.append([y+1, x])
            q.append([y+1, x, path+'S'])
            dead_end = False
            
        if known_maze[y][x-1] in ['.', 'E'] and [y, x-1] not in visit:
            visit.append([y, x-1])
            q.append([y, x-1, path+'A'])
            dead_end = False
            
        if known_maze[y][x+1] in ['.', 'E'] and [y, x+1] not in visit:
            visit.append([y, x+1])
            q.append([y, x+1, path+'D'])
            dead_end = False
        
        # if start point
        if path == '':
            continue
        
        if dead_end:
            reverse_path = get_reverse_path(path[:-1], q[-1][2])
            send_move(reverse_path, s)
            continue
        
        # sending path to server
        move = path[-1]
        new_maze = send_move(move, s)
        
        if move == 'W':            
            new_row = new_maze[0][1].replace(' ', '')
            new_row_idx = new_maze[0][0]
            
            left = x - vpw // 2 if x - vpw // 2 >= 0 else 0
            right = x + vpw // 2 + 1 if x + vpw // 2 + 1 <= len(known_maze[0]) else len(known_maze[0])            
            origin_row = known_maze[new_row_idx]
            
            if origin_row[left:right] != new_row: 
                new_row_str = origin_row[:left] + new_row + origin_row[right:]
                known_maze[new_row_idx] = new_row_str
            
        if move == 'S':            
            new_row = new_maze[-1][1].replace(' ', '')
            new_row_idx = new_maze[-1][0]
            
            left = x - vpw // 2 if x - vpw // 2 >= 0 else 0
            right = x + vpw // 2 + 1 if x + vpw // 2 + 1 <= len(known_maze[0]) else len(known_maze[0])            
            origin_row = known_maze[new_row_idx]
            
            
            if origin_row[left:right] != new_row: 
                new_row_str = origin_row[:left] + new_row + origin_row[right:]
                known_maze[new_row_idx] = new_row_str
            
        if move == 'A':            
            y_offset = new_maze[0][0]
            y_len = len(new_maze)
            new_col_idx = x - (vpw // 2)
                
            if new_maze[0][1].startswith(' '): # reach the left edge of the maze
                tmp_idx = 0
                
                for i, row in enumerate(known_maze):
                    if y_offset <= i < y_offset + y_len:
                        new_row = new_maze[tmp_idx][1].replace(' ', '')[0] + row[1:]
                        known_maze[i] = new_row
                        tmp_idx += 1
            else:
                if new_col_idx == -1:
                    for v in visit:
                        v[1] += 1
                    for p in q:
                        p[1] += 1
                    
                    tmp_idx = 0
                    
                    for i, row in enumerate(known_maze):
                        if y_offset <= i < y_offset + y_len:
                            new_row = new_maze[tmp_idx][1].replace(' ', '')[0] + row
                            tmp_idx += 1
                        else:
                            new_row = PAD + row
                        known_maze[i] = new_row
                else:
                    tmp_idx = 0
                    for i, row in enumerate(known_maze):
                        if y_offset <= i < y_offset + y_len:
                            new_row = row[:new_col_idx] + new_maze[tmp_idx][1].replace(' ', '')[0] + row[new_col_idx + 1:]
                            known_maze[i] = new_row
                            tmp_idx += 1
                        
        if move == 'D':
            y_offset = new_maze[0][0]
            y_len = len(new_maze)
            new_col_idx = x + (vpw // 2)
            
            if new_maze[0][1].endswith(' '): # right edge of the maze
                tmp_idx = 0

                for i, row in enumerate(known_maze):
                    if y_offset <= i < y_offset + y_len:
                        new_row = row[:-1] + new_maze[tmp_idx][1].replace(' ', '')[-1]
                        known_maze[i] = new_row
                        tmp_idx += 1
            else:
                if new_col_idx == len(known_maze[0]):
                    tmp_idx = 0
                    
                    for i, row in enumerate(known_maze):
                        if y_offset <= i < y_offset + y_len:
                            new_row = row + new_maze[tmp_idx][1].replace(' ' ,'')[-1]
                            tmp_idx += 1
                        else:
                            new_row = row + PAD
                        known_maze[i] = new_row
                else:
                    tmp_idx = 0
                    for i, row in enumerate(known_maze):
                        if y_offset <= i < y_offset + y_len:
                            new_row = row[:new_col_idx] + new_maze[tmp_idx][1].replace(' ', '')[-1] + row[new_col_idx + 1:]
                            known_maze[i] = new_row
                            tmp_idx += 1
        
        for km in enumerate(known_maze):
            print(f'{km}')
        print()
    
    return flag


if __name__ == '__main__':
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('inp.zoolab.org', 10304))
    # s.settimeout(0.5)
    
    while True:
        data = s.recv(1024)
        info = data.decode('utf-8')
        print('info\n', info)
        maze_meta = info.find('Note1: Size')
        maze_vp = info.find('Note2: View')
        if (maze_meta == -1 or maze_vp == -1):
            continue        
        
        width = int(info[maze_meta+26:maze_meta+29])
        height = int(info[maze_meta+32:maze_meta+35])

        vpw = int(info[maze_vp+24:maze_vp+26])
        vph = int(info[maze_vp+29:maze_vp+31])
        
        info = info[maze_vp+31:]
        pre_maze = info.split('\n')
        
        flag = find_path(pre_maze, (height, width), (vph, vpw), s)
        print(flag)
        break  
    