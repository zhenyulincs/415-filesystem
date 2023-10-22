import os


class test_base:
    def __init__(self,test_name):
        self.LOG_FILE = "log.txt"
        self.test_name = test_name
        if (os.path.isfile(self.LOG_FILE)):
            os.remove(self.LOG_FILE)
        print("Testing Starting...........")
        os.system('clear')
        print("screen clear done!")
        os.system('make clean')
        print("make clean done!")

    def write_to_log(self,content):
        with open(self.LOG_FILE,'a+') as f:
            f.write(f"============={self.test_name} test==============\n")
            f.write(content)

    def clear_volume(self):
        os.system('rm SampleVolume')
        print("remove SampleVolume done!")
