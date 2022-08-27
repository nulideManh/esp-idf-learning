# NCP Host Library source code
## 1, Install [ESP IDF SDK](https://github.com/espressif/esp-idf) on Ubuntu 18 (or above)
### The sdk use in this project depend on your sdk main project

```bash
$ sudo apt-get install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

```bash
$ git clone  https://github.com/espressif/esp-idf.git ~/esp/
$ cd ~/esp/esp-idf
$ git submodule update --init --recursive
$ ./install.sh
```

## 2, Clone project 

```bash
$ git clone https://gitlab.com/hoangvanloc.cdt1k4/esp_ncp_blemesh.git
```
## 3, Build library

```bash
$ . ~/esp/esp-idf/export.sh
$ idf.py build
```
