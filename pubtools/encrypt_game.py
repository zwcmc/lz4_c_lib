#coding:utf-8
import sys
import os, os.path
import shutil
import struct

reload(sys)
sys.setdefaultencoding('utf-8')

def lstFilesByDir(_dir,_func=None,recursion=True):
    dirs = os.listdir(_dir)
    for item in dirs:
        fullPath = os.path.join(_dir,item)
        if os.path.isdir(fullPath):
            if recursion:
                lstFilesByDir(fullPath,_func,recursion)
        else:
            if _func:
                _func(fullPath)

def funcXXTEA(path1, path2):
    command = ""
    if sys.platform == 'win32':
        command = "call xxtea encrypt god key_file=key {0} {1}".format(path1,path2)
    else:
        command = "./xxtea encrypt god key_file=key {0} {1}".format(path1,path2)
    retCommand = os.system(command)
    if  retCommand != 0:
        raise Exception("error:{0}".format(path1))

def funcLz4fCompress(path1, path2):
    if sys.platform == 'win32':
        command = 'call lz4 {0} {1}'.format(path1, path2)
        retCommand = os.system(command)
        if retCommand != 0:
            raise Exception("error:{0}".format(path1))

def compressFunc(path):
    fileName = os.path.basename(path)
    arr = os.path.splitext(fileName)
    fileDir =os.path.dirname(path)
    if len(arr) != 2:
        return
    if arr[1] == ".lua" or arr[1] == ".json" or arr[1] == ".plist" or arr[1] == ".ExportJson":
        temp1Path = os.path.join(fileDir, arr[0] + '_temp1') + arr[1]
        temp2Path = os.path.join(fileDir, arr[0] + '_temp2') + arr[1]
        funcLz4fCompress(path, temp1Path)
        file_1 = open(path, 'rb')
        file_2 = open(temp1Path, 'rb')
        ret_1 = file_1.read()
        ret_2 = file_2.read()
        file_3 = open(temp2Path, 'ab')
        markNum = 19911106
        file_3.write(struct.pack('I', markNum))
        file_3.write(struct.pack('I', len(ret_1)))
        file_3.write(ret_2)
        file_1.close()
        file_2.close()
        file_3.close()
        os.remove(path)
        os.remove(temp1Path)
        os.rename(temp2Path, path)

def func(path):
    fileName = os.path.basename(path)
    arr = os.path.splitext(fileName)
    fileDir =os.path.dirname(path)
    if len(arr)!=2:
        return
    if arr[1]==".lua":
        funcXXTEA(path,os.path.join(fileDir,arr[0])+".luac")
        os.remove(path)
    elif arr[1] == ".png" or arr[1] == ".jpg" or arr[1] == ".json" or arr[1] == ".plist" or arr[1] == ".ExportJson":
        funcXXTEA(path,path)

lstFilesByDir(sys.argv[1], compressFunc)
lstFilesByDir(sys.argv[1], func)
