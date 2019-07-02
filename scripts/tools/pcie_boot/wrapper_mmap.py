import os
import ctypes

class wrapper_mmap():
    so:ctypes.CDLL
    filename:str = './temp_mmap.c'
    libraryname:str = './libtemp_mmap.so'
    source_code = [
        '#include <stdio.h>\n',
        '#include <unistd.h>\n',
        '#include <fcntl.h>\n',
        '#include <sys/types.h>\n',
        '#include <sys/mman.h>\n',
        '#define u32 unsigned int\n',
        '#define TRUE 0\n',
        '#define FALSE 1\n',
        'static void *pci_mmap_addr;\n',
        'int detach_pci_resource(void)\n',
        '{\n',
        '   int result = munmap(pci_mmap_addr, getpagesize());\n',
        '   if (result < 0){\n',
        '       return FALSE;\n',
        '   } else {\n',
        '       return TRUE;\n',
        '   }\n',
        '}\n',
        'int attach_pci_resource(char *device)\n',
        '{\n',
        '   int pci_fd;\n',
        '   char path[512];\n',
        '   pci_fd = open(device, O_RDWR);\n',
        '   if (pci_fd >= 0){\n',
        '       pci_mmap_addr = mmap(0, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, pci_fd, 0);\n',
        '       close(pci_fd);\n',
        '       if (pci_mmap_addr == MAP_FAILED){\n',
        '           return FALSE;\n',
        '       }else{\n',
        '           return TRUE;\n',
        '       }\n',
        '   }\n',
        '   return FALSE;\n',
        '}\n',
        'u32 write_register(u32 byte_offset, u32 val)\n',
        '{\n',
        '   u32 *reg_addr = (u32 *)(pci_mmap_addr + byte_offset);\n',
        '   *reg_addr = val;\n',
        '   return *reg_addr;\n',
        '}\n',
        'u32 read_register(u32 byte_offset)\n',
        '{\n',
        '   u32 *reg_addr = (u32 *)(pci_mmap_addr + byte_offset);\n',
        '   return *reg_addr;\n',
        '}\n',
    ]

    def __init__(self):
        pass

    def create_src(self):
        with open(self.filename, 'w') as file:
            file.writelines(self.source_code)

    def build_so(self):
        import subprocess
        build_msg = subprocess.check_output('cc -fPIC -shared -o {} {}'.format(self.libraryname, self.filename), shell=True)
        if build_msg.decode('utf-8') != '':
            print("build_sharedlibrary error!")
            exit(-1)

    def remove_files(self):
        import os
        os.remove(self.filename)
        os.remove(self.libraryname)

    def __enter__(self):
        self.create_src()
        self.build_so()
        self.load_so()
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        self.remove_files()

    def load_so(self):
        self.so = ctypes.CDLL(self.libraryname)

    def read_register(self, dw_offset:int):
        return ctypes.c_uint(self.so.read_register(dw_offset)).value

    def write_register(self, dw_offset:int, value:int):
        return self.so.write_register(dw_offset, value)
 
    def attach_pci(self, device:str):
        SUCCESS = 0
        FALSE = 1
        if self.so.attach_pci_resource(device.encode()) == SUCCESS:
            return True
        else :
            return False

    def detach_pci(self):
        SUCCESS = 0
        FALSE = 1
        if self.so.detach_pci_resource() == SUCCESS:
            return True
        else :
            return False

