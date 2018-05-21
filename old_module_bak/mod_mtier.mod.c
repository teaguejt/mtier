#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x6ae5c4e6, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x60ba3eb, __VMLINUX_SYMBOL_STR(param_ops_ulong) },
	{ 0x75c372e4, __VMLINUX_SYMBOL_STR(__free_pages) },
	{ 0xe511078c, __VMLINUX_SYMBOL_STR(kthread_stop) },
	{ 0x58a91a03, __VMLINUX_SYMBOL_STR(wake_up_process) },
	{ 0x8e96f07c, __VMLINUX_SYMBOL_STR(kthread_create_on_node) },
	{ 0xb16c780d, __VMLINUX_SYMBOL_STR(alloc_page_interleave) },
	{ 0xc0a3d105, __VMLINUX_SYMBOL_STR(find_next_bit) },
	{ 0xb352177e, __VMLINUX_SYMBOL_STR(find_first_bit) },
	{ 0xf99d347e, __VMLINUX_SYMBOL_STR(node_states) },
	{ 0xc87c1f84, __VMLINUX_SYMBOL_STR(ktime_get) },
	{ 0xf9a482f9, __VMLINUX_SYMBOL_STR(msleep) },
	{ 0xb3f7646e, __VMLINUX_SYMBOL_STR(kthread_should_stop) },
	{ 0x84ec0e26, __VMLINUX_SYMBOL_STR(init_task) },
	{ 0xac1dccef, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x4185f6d7, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x6f0b27c8, __VMLINUX_SYMBOL_STR(mmput) },
	{ 0x189868d7, __VMLINUX_SYMBOL_STR(get_random_bytes_arch) },
	{ 0x92888ced, __VMLINUX_SYMBOL_STR(end_page_writeback) },
	{ 0x441eb48, __VMLINUX_SYMBOL_STR(ksm_migrate_page) },
	{ 0xa984be87, __VMLINUX_SYMBOL_STR(page_cpupid_xchg_last) },
	{ 0x5456e2c5, __VMLINUX_SYMBOL_STR(mtier_set_page_idle) },
	{ 0xc439e91f, __VMLINUX_SYMBOL_STR(mtier_page_is_idle) },
	{ 0x3c5ce7cd, __VMLINUX_SYMBOL_STR(mtier_set_page_young) },
	{ 0x3da8cb3b, __VMLINUX_SYMBOL_STR(mtier_page_is_young) },
	{ 0x521445b, __VMLINUX_SYMBOL_STR(list_del) },
	{ 0x343a1a8, __VMLINUX_SYMBOL_STR(__list_add) },
	{ 0xd6ee688f, __VMLINUX_SYMBOL_STR(vmalloc) },
	{ 0x999e8297, __VMLINUX_SYMBOL_STR(vfree) },
	{ 0xfdd27b1b, __VMLINUX_SYMBOL_STR(mtier_get_ro_pagelist) },
	{ 0xbd6f03f7, __VMLINUX_SYMBOL_STR(putback_movable_pages) },
	{ 0x7ff92bfb, __VMLINUX_SYMBOL_STR(get_task_mm) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0xab18162b, __VMLINUX_SYMBOL_STR(__put_task_struct) },
	{ 0x68b43ee1, __VMLINUX_SYMBOL_STR(security_task_movememory) },
	{ 0xc6cbbc89, __VMLINUX_SYMBOL_STR(capable) },
	{ 0x55ea103, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "87E620D6F405E4D27DB4597");
