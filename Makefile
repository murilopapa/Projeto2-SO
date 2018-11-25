#	Leonardo Carbonari        	13126578
#	Matheus Franceschini        13129788
#	Pedro Tortella            	13035555
#	Tales Falcão            	13146394
#	Diogo Esteves Furtado     	15153927
#	Kaíque Ferreira Fávero    	15118698


obj-$(CONFIG_MINIX_FS) += minix.o

minix-objs := bitmap.o itree_v1.o itree_v2.o namei.o inode.o file.o dir.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
