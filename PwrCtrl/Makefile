#master
ifeq ($(pc),y)
	CROSS  =
	include config.pc
else
	CROSS  = arm-linux-gnueabihf-
	include config.arm
endif

CC    := $(CROSS)gcc

INCLUDES = -I../../libs \
           -I../../thirdpart/dbus_$(MARCH) \
           -I../../thirdpart/zmq_$(MARCH)/include \
           -I../../thirdpart/openssl_$(MARCH)/include \
           -I../../thirdpart/curl_$(MARCH)/include \
           -I../../thirdpart/ssh2_$(MARCH)/include \
           -I./src/lib \
           -I./src/lib/aes \
           -I./src/lib/export \
           -I./src/dbus \
           -I./src/pwrCtrl \
           -I./src/modCtrl \
           -I./src/fanCtrl \
           -I./src/relayCtrl \
           -I./src/fault \
           -I./src/smartOps \
           -I./src/zmq \
           -I./src/conf

LIBS     = -L../../thirdpart/dbus_$(MARCH) \
		   -L../../thirdpart/zmq_$(MARCH)/lib \
		   -L../../thirdpart/openssl_$(MARCH)/lib \
           -L../../thirdpart/curl_$(MARCH)/lib \
           -L../../thirdpart/ssh2_$(MARCH)/lib \
		   -L../../rootfs/lib \
		   -L../../libs/$(MARCH) \
	       -ldbus-1 -lsc -lpthread -lrt -lzmq -lcrypto -lcurl -lssh2 -lssl

APP      = build/pwrproc
#CFLAGS   = -O2 -Wall -c -fmessage-length=0 -Wno-deprecated-declarations $(INCLUDES)
CFLAGS   = -O0 -g3 -Wall -c -fmessage-length=0 -Wno-deprecated-declarations $(INCLUDES)
LDFLAGS  = $(LIBS)

SRCDIR = ./src
OBJDIR = ./build

SRCS = $(shell find  $(SRCDIR) -name "*.c"  )
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS)) 
DEPS = $(patsubst %.o, %.d, $(OBJS)) 

INSTALL_BIN = ../../ubi_root/local/apps/pwrctrl/
INSTALL_CONFIG = $(INSTALL_BIN)

PCINSTALL = /ubi/local/apps/pwrctrl
#MYSRCDIR = /root/share/comsys/apps/19-A-Chgctrl
MYSRCDIR = ../19-A-Pwrctrl

all : $(APP)

-include $(DEPS)

$(APP) : $(OBJS)
	@$(CC) -o $(APP) $(OBJS) $(LDFLAGS)  -Wl,-Map,pwrctrl.map
	@echo "[Building]  "$@
	@echo "[Install]   "$@" to "$(INSTDIR)
	@[ ! -e $(INSTDIR) ] &  mkdir -p $(INSTDIR)
	@-cp $(APP) $(INSTDIR) 
	@echo "[Completed] Have fun !  ^-^" 

install : $(APP)
	@echo "[Install]   "$@" to "$(INSTDIR)
	@-cp $(APP) $(INSTDIR) 
	@echo "[Completed] Have fun !  ^-^" 

dstinstall : $(APP)
	@echo "install to rootfs"
	@[ -d $(INSTALL_BIN) ] || mkdir $(INSTALL_BIN)
	@cp $(APP) $(INSTALL_BIN)
	@cp ./src/conf/pwrstart.sh $(INSTALL_BIN)
	@[ -d $(INSTALL_CONFIG) ] || mkdir $(INSTALL_CONFIG)
	@cp ./src/conf/pwrCnf.json $(INSTALL_CONFIG)
	@cp ./src/conf/smartOps.json $(INSTALL_CONFIG)
	
pcinstall :
	@-ln -s  $(MYSRCDIR)/build/pwrproc $(PCINSTALL)/pwrproc
	@-ln -s  $(MYSRCDIR)/src/conf/pwrCnf.json $(PCINSTALL)/pwrCnf.json
	@-ln -s  $(MYSRCDIR)/src/conf/pwrCnf.json /ubi/conf/pwrctrl/pwrCnf.json

padinstall : $(APP)
	@echo "install to pad"
	@cp $(APP) /home/zxk/nfshare/

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@[ ! -e $(dir $@) ] &  mkdir -p $(dir $@)
	@echo "[Compiling] "$@
	@$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.d: $(SRCDIR)/%.c
	@[ ! -e $(dir $@) ] &  mkdir -p $(dir $@)
	@$(CC) -MM $(CFLAGS) $< > $@.tmp
	@sed 's,\(.*\)\.o[ ]*:,$(dir $@)\1.o :,g' < $@.tmp > $@;
	@rm -f $@.tmp

clean:
	rm -rf $(OBJS) $(DEPS) $(APP)
