/*
	Murilo Martos Mendonça
	Joao Pedro Favara
	Victor Hugo Nascimento
*/

#include "minix.h"
#include <asm/uaccess.h> // Needed by segment descriptors
#include <linux/export.h>
#include <linux/compiler.h>
#include <linux/dax.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/capability.h>
#include <linux/kernel_stat.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/mman.h>
#include <linux/pagemap.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/hash.h>
#include <linux/writeback.h>
#include <linux/backing-dev.h>
#include <linux/pagevec.h>
#include <linux/blkdev.h>
#include <linux/security.h>
#include <linux/cpuset.h>
#include <linux/hardirq.h> /* for BUG_ON(!in_atomic()) only */
#include <linux/hugetlb.h>
#include <linux/memcontrol.h>
#include <linux/cleancache.h>
#include <linux/rmap.h>

#include <linux/slab.h> //kmalloc
#include <linux/crypto.h>
#include <crypto/sha.h>
#include <crypto/internal/hash.h>
#include <linux/scatterlist.h>
#include <asm/uaccess.h>
/*
 * We have mostly NULLs here: the current defaults are OK for
 * the minix filesystem.
 */

#define CRYPTO_BUFFER_SIZE 32

char *cryptoKey = "ABCDEF0123456789";

ssize_t read_sob(struct kiocb *iocb, struct iov_iter *iter)
{
	if (strlen((char *)(iter->iov->iov_base)) != 0)
	{
		printk("FUNCAO READ");
		int size = iter->iov->iov_len;
		printk("size de entrada: %d", size);
		char test[size];
		int i;

		strlcpy(test, iter->iov->iov_base, 32);
		printk("variavel recebida em hexa: %*ph", size, test);

		struct crypto_cipher *tmf;
		char dest[CRYPTO_BUFFER_SIZE];

		tmf = crypto_alloc_cipher("aes", 4, 32);
		crypto_cipher_setkey(tmf, cryptoKey, 32);

		crypto_cipher_decrypt_one(tmf, dest, test);
		crypto_cipher_decrypt_one(tmf, &dest[16], &test[16]);

		crypto_free_cipher(tmf);

		printk("Resultado var dest: %s", dest);
		strlcpy(iter->iov->iov_base, dest, 32);
		printk("Resultado vat iter: %s", iter->iov->iov_base);
	}
	return generic_file_read_iter(iocb, iter);
}

ssize_t write_sob(struct kiocb *iocb, struct iov_iter *from)
{
	printk("FUNCAO WRITE");

	int size = from->iov->iov_len;
	char *buf = kmalloc(size, GFP_KERNEL);
	printk("size de from->iov->iov_base:%d", size);
	strlcpy(buf, from->iov->iov_base, size);
	printk("buf com from->iov->iov_base:%s", buf);

	struct crypto_cipher *tmf;
	char dest[CRYPTO_BUFFER_SIZE];
	char dec[CRYPTO_BUFFER_SIZE];
	int i;

	/* Begin Crypto */
	tmf = crypto_alloc_cipher("aes", 4, 32);
	crypto_cipher_setkey(tmf, cryptoKey, 32);

	crypto_cipher_encrypt_one(tmf, dest, buf);
	crypto_cipher_encrypt_one(tmf, &dest[16], &buf[16]);

	printk(KERN_ALERT "Crypted: %*ph", 32, dest);

	crypto_cipher_decrypt_one(tmf, dec, dest);
	crypto_cipher_decrypt_one(tmf, &dec[16], &dest[16]);

	printk(KERN_ALERT "Decrypted: %s", dec);

	crypto_free_cipher(tmf);

	/* End Crypto */
	printk(KERN_ALERT "from->iov->iov_base recebida: %s", from->iov->iov_base);
	memcpy(from->iov->iov_base, dest, 32);
	printk(KERN_ALERT "from->iov->iov_base modificada string: %s", from->iov->iov_base);
	printk(KERN_ALERT "from->iov->iov_base modificada hexa: %*ph", 32, from->iov->iov_base);

	return generic_file_write_iter(iocb, from);
}

/*
 * We have mostly NULLs here: the current defaults are OK for
 * the minix filesystem.
 */
const struct file_operations minix_file_operations = {
	.llseek = generic_file_llseek,
	.read_iter = read_sob,
	.write_iter = write_sob,
	.mmap = generic_file_mmap,
	.fsync = generic_file_fsync,
	.splice_read = generic_file_splice_read,
};

static int minix_setattr(struct dentry *dentry, struct iattr *attr)
{
	struct inode *inode = d_inode(dentry);
	int error;

	error = setattr_prepare(dentry, attr);
	if (error)
		return error;

	if ((attr->ia_valid & ATTR_SIZE) &&
		attr->ia_size != i_size_read(inode))
	{
		error = inode_newsize_ok(inode, attr->ia_size);
		if (error)
			return error;

		truncate_setsize(inode, attr->ia_size);
		minix_truncate(inode);
	}

	setattr_copy(inode, attr);
	mark_inode_dirty(inode);
	return 0;
}

const struct inode_operations minix_file_inode_operations = {
	.setattr = minix_setattr,
	.getattr = minix_getattr,
};
