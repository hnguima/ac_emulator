import os
import gzip
import stat
import shutil

server = './http_server'

ret = 0

if(os.name == "posix"):
    ret = os.system(f'rm -r "./build/{server}"')  
elif(os.name == "nt"):
    ret = os.system(f'rmdir /s /q "./build/{server}"')  

# if(ret != 0):
#     print(f'Erro ao remover a pasta {server}')
#     quit()
    

print('walk_dir (absolute) = ' + os.path.abspath(server))
for root, subdirs, files in os.walk(server):
    os.mkdir(f'./build/{root}')
    for file in files:
        print(f'compressing {root}/{file}')
        with open(f'{root}/{file}', 'rb') as f_in, gzip.open(f'./build/{root}/{file}', 'wb') as f_out:
            f_out.writelines(f_in)
        