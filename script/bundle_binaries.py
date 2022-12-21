import shutil
import os

import json

import git

if os.path.exists("./release"):
    shutil.rmtree('./release')

os.makedirs("./release")

repo = git.Repo(search_parent_directories=True)

prj_version = "0.0.0"
if (len(repo.tags) != 0):
    prj_version = str(repo.tags[-1])

os.chdir('./build')
with open('project_description.json') as prj_desc:
    prj_name = json.load(prj_desc)["project_name"]
    
os.system("esptool.py --chip ESP32 merge_bin -o ../release/" + prj_name + "_" + prj_version + ".bin @flash_args")

os.chdir('../release/')
with open('command.txt', 'w') as command_file:
    command_file.write("esptool.py -p COM## -b 500000 --before default_reset --after hard_reset --chip esp32 write_flash --flash_mode dio --flash_freq 40m --flash_size detect 0 " + prj_name + "_" + prj_version + ".bin")
