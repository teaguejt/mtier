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
	{ 0xfd6a61af, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x60ba3eb, __VMLINUX_SYMBOL_STR(param_ops_ulong) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xf188dbc, __VMLINUX_SYMBOL_STR(__free_pages) },
	{ 0x65ec0b8f, __VMLINUX_SYMBOL_STR(kthread_stop) },
	{ 0x5917f0bd, __VMLINUX_SYMBOL_STR(alloc_page_interleave) },
	{ 0xea31cd4b, __VMLINUX_SYMBOL_STR(wake_up_process) },
	{ 0x675f005c, __VMLINUX_SYMBOL_STR(kthread_create_on_node) },
	{ 0xc0a3d105, __VMLINUX_SYMBOL_STR(find_next_bit) },
	{ 0xb352177e, __VMLINUX_SYMBOL_STR(find_first_bit) },
	{ 0xf99d347e, __VMLINUX_SYMBOL_STR(node_states) },
	{ 0x189868d7, __VMLINUX_SYMBOL_STR(get_random_bytes_arch) },
	{ 0xd6ee688f, __VMLINUX_SYMBOL_STR(vmalloc) },
	{ 0x999e8297, __VMLINUX_SYMBOL_STR(vfree) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x13670f7b, __VMLINUX_SYMBOL_STR(__put_task_struct) },
	{ 0x78e739aa, __VMLINUX_SYMBOL_STR(up) },
	{ 0xa9c18275, __VMLINUX_SYMBOL_STR(mmput) },
	{ 0xa2e8a777, __VMLINUX_SYMBOL_STR(mtier_get_ro_pagelist) },
	{ 0xbd6f03f7, __VMLINUX_SYMBOL_STR(putback_movable_pages) },
	{ 0x7144f3b7, __VMLINUX_SYMBOL_STR(get_task_mm) },
	{ 0xbb7d95de, __VMLINUX_SYMBOL_STR(security_task_movememory) },
	{ 0xc6cbbc89, __VMLINUX_SYMBOL_STR(capable) },
	{ 0xf9b2a6f, __VMLINUX_SYMBOL_STR(down_trylock) },
	{ 0xf9a482f9, __VMLINUX_SYMBOL_STR(msleep) },
	{ 0x2d63fb3a, __VMLINUX_SYMBOL_STR(init_task) },
	{ 0xb3f7646e, __VMLINUX_SYMBOL_STR(kthread_should_stop) },
	{ 0xc4d5c571, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0x5f40bd26, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x512f77c, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xc87c1f84, __VMLINUX_SYMBOL_STR(ktime_get) },
	{ 0x417f0dde, __VMLINUX_SYMBOL_STR(up_read) },
	{ 0xac09a022, __VMLINUX_SYMBOL_STR(unlock_page) },
	{ 0x61e284ca, __VMLINUX_SYMBOL_STR(mtier_trylock_page) },
	{ 0xc109f58f, __VMLINUX_SYMBOL_STR(anon_vma_interval_tree_iter_next) },
	{ 0xe04c731c, __VMLINUX_SYMBOL_STR(anon_vma_interval_tree_iter_first) },
	{ 0xe2b3a0f7, __VMLINUX_SYMBOL_STR(down_read) },
	{ 0x1aae58c, __VMLINUX_SYMBOL_STR(mtier_set_page_young) },
	{ 0x50fbf405, __VMLINUX_SYMBOL_STR(mtier_set_page_idle) },
	{ 0x343a1a8, __VMLINUX_SYMBOL_STR(__list_add) },
	{ 0x521445b, __VMLINUX_SYMBOL_STR(list_del) },
	{ 0x93f276f6, __VMLINUX_SYMBOL_STR(end_page_writeback) },
	{ 0xcac19f2c, __VMLINUX_SYMBOL_STR(ksm_migrate_page) },
	{ 0xa984be87, __VMLINUX_SYMBOL_STR(page_cpupid_xchg_last) },
	{ 0x9639b0b8, __VMLINUX_SYMBOL_STR(mtier_page_is_idle) },
	{ 0x562665a9, __VMLINUX_SYMBOL_STR(mtier_page_is_young) },
	{ 0x6bf1c17f, __VMLINUX_SYMBOL_STR(pv_lock_ops) },
	{ 0xe259ae9e, __VMLINUX_SYMBOL_STR(_raw_spin_lock) },
	{ 0xdad51e41, __VMLINUX_SYMBOL_STR(pv_mmu_ops) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "4B16A3FF3009A252C23CCDB");
