---
title: Linux内核内存管理核心问题
date: 2025-03-13 16:47:35
tags:
- 原创

---

### Linux内核内存管理核心问题

> 以下是忘记哪位大师提出的核心问题，尝试在AI的帮助下进行解答

1. **<font style="color:rgb(0, 0, 0);">在系统启动时，ARM Linux内核如何知道系统中有多大的内存空间？</font>**

AI 答：内存硬件信息写在设备树文件中，bootloader读取设备树文件，并传递给内核，内核解析设备树文件，动态获取硬件系统内存信息。

```bash
+-------------------+       +-------------------+       +-------------------+
|   BootLoader      |       |   Linux Kernel    |       |   Hardware        |
+-------------------+       +-------------------+       +-------------------+
| 1. 从存储介质读取 |  --->  | 2. 解析设备树文件 |  --->  | 3. 初始化设备     |
|    设备树文件     |       |    (DTB)          |       |                   |
| 4. 传递给内核     |  --->  | 4. 动态配置       |       |                   |
+-------------------+       +-------------------+       +-------------------+
```

```bash
dmesg | grep Memory
[    0.081932] Memory: 7713716K/8134900K available (16393K kernel code, 4392K rwdata, 10880K rodata, 3372K init, 18700K bss, 420924K reserved, 0K cma-reserved)
[    0.164957] x86/mm: Memory block size: 128M
```



2. **在32bit Linux内核中，用户空间和内核空间的比例通常是3:1，可以修改成2:2吗？**

答：可以的，通过menuconfig。

1. <font style="color:rgb(64, 64, 64);">进入</font><font style="color:rgb(64, 64, 64);"> </font>**<font style="color:rgb(64, 64, 64);">Processor type and features</font>**<font style="color:rgb(64, 64, 64);"> </font><font style="color:rgb(64, 64, 64);">→</font><font style="color:rgb(64, 64, 64);"> </font>**<font style="color:rgb(64, 64, 64);">Memory split</font>**<font style="color:rgb(64, 64, 64);">。</font>
2. <font style="color:rgb(64, 64, 64);">选择 </font>**<font style="color:rgb(64, 64, 64);">2G/2G user/kernel split</font>**<font style="color:rgb(64, 64, 64);">（即</font>`<font style="color:rgb(64, 64, 64);">CONFIG_VMSPLIT_2G</font>`<font style="color:rgb(64, 64, 64);">）。</font>
3. <font style="color:rgb(64, 64, 64);">然后编译新内核</font>

```bash
choice
	prompt "Memory split"
	depends on MMU
	default VMSPLIT_3G
	help
	  Select the desired split between kernel and user memory.

	  If you are not absolutely sure what you are doing, leave this
	  option alone!

	config VMSPLIT_3G
		bool "3G/1G user/kernel split"
	config VMSPLIT_3G_OPT
		depends on !ARM_LPAE
		bool "3G/1G user/kernel split (for full 1G low memory)"
	config VMSPLIT_2G
		bool "2G/2G user/kernel split"
	config VMSPLIT_1G
		bool "1G/3G user/kernel split"
endchoice
```

---

4. **<font style="color:rgb(0, 0, 0);">物理内存页面如何添加到伙伴系统中，是一页一页添加，还是以2的几次幂来加入呢？</font>**

<font style="color:rgb(0, 0, 0);">答： </font><font style="color:rgb(6, 6, 7);">在伙伴系统中，物理内存页面是以2的幂次方大小的块的形式添加到系统中的。这种设计使得内存管理更加高效，同时也便于内存块的合并和拆分操作</font>

> <font style="color:rgb(6, 6, 7);">在Linux内核的伙伴系统（Buddy System）中，物理内存页面的分配和回收是以2的幂次方大小的块为单位进行管理的。具体来说，内存页面的添加（回收）到伙伴系统中是以2的幂次方大小的块来加入的，而不是一页一页单独添加的</font>
>
> ### <font style="color:rgb(6, 6, 7);">伙伴系统的基本原理</font>
> <font style="color:rgb(6, 6, 7);">伙伴系统是一种内存分配算法，主要用于管理物理内存页面。它将内存划分为大小为2的幂次方的块（如1页、2页、4页、8页……），每个块称为一个“伙伴”。当需要分配内存时，系统会根据请求的大小找到合适大小的伙伴块；当释放内存时，系统会将释放的内存块重新加入到伙伴系统中。</font>
>
> ### <font style="color:rgb(6, 6, 7);">内存页面的添加方式</font>
> 1. **<font style="color:rgb(6, 6, 7);">内存块的大小</font>**<font style="color:rgb(6, 6, 7);">：</font>
>     - <font style="color:rgb(6, 6, 7);">伙伴系统中的内存块大小必须是2的幂次方。例如，1页、2页、4页、8页、16页等。</font>
>     - <font style="color:rgb(6, 6, 7);">这是因为伙伴系统的合并和拆分操作依赖于这种幂次方的大小，能够高效地进行内存管理。</font>
> 2. **<font style="color:rgb(6, 6, 7);">内存块的回收</font>**<font style="color:rgb(6, 6, 7);">：</font>
>     - <font style="color:rgb(6, 6, 7);">当一个进程释放内存时，它会将释放的内存块以2的幂次方大小的块的形式返回给伙伴系统。</font>
>     - <font style="color:rgb(6, 6, 7);">例如，如果一个进程释放了一个8页大小的内存块，这个8页的块会被直接添加到伙伴系统中，而不是一页一页地分别添加。</font>
> 3. **<font style="color:rgb(6, 6, 7);">合并机制</font>**<font style="color:rgb(6, 6, 7);">：</font>
>     - <font style="color:rgb(6, 6, 7);">如果释放的内存块与伙伴系统中已有的相邻内存块大小相同，它们可以合并成一个更大的块。</font>
>     - <font style="color:rgb(6, 6, 7);">例如，如果伙伴系统中已经有一个4页的块，而释放的内存块也是一个4页的块，并且它们在物理内存中是相邻的，那么它们可以合并成一个8页的块。</font>
>
> ### <font style="color:rgb(6, 6, 7);">为什么不是一页一页添加</font>
> <font style="color:rgb(6, 6, 7);">如果一页一页地添加内存到伙伴系统中，会导致以下问题：</font>
>
> + **<font style="color:rgb(6, 6, 7);">效率低下</font>**<font style="color:rgb(6, 6, 7);">：一页一页地添加会增加管理开销，因为每次添加都需要更新伙伴系统的状态。</font>
> + **<font style="color:rgb(6, 6, 7);">碎片化问题</font>**<font style="color:rgb(6, 6, 7);">：一页一页地添加会增加内存碎片化的风险，导致难以找到足够大的连续内存块来满足较大内存分配请求。</font>
>
> <font style="color:rgb(6, 6, 7);">通过以2的幂次方大小的块来管理内存，伙伴系统能够高效地进行内存分配和回收，同时减少内存碎片化。</font>
>



1. <font style="color:rgb(0, 0, 0);">内核的一级页表存放在什么地方？二级页表又存放在什么地方？</font>

> [需确认] 一级页表与二级页表存在物理内存中，<font style="color:rgb(6, 6, 7);">一级页表的基地址存放在TTBRx寄存器中</font>
>

2. <font style="color:rgb(0, 0, 0);">用户进程的一级页表存放在什么地方？二级页表呢？</font>

:::info
<font style="color:rgb(64, 64, 64);">用户进程的一级页表和二级页表的存储位置及管理方式如下：</font>

### **<font style="color:rgb(64, 64, 64);">一级页表（如页目录）</font>**
1. **<font style="color:rgb(64, 64, 64);">物理内存中</font>**<font style="color:rgb(64, 64, 64);">  
</font><font style="color:rgb(64, 64, 64);">一级页表（例如x86架构中的页目录）存储在物理内存中，由操作系统内核负责分配和管理。每个进程在创建时，内核会为其分配一个独立的页目录，用于记录该进程的虚拟地址到物理地址的映射关系。</font>
2. **<font style="color:rgb(64, 64, 64);">通过CR3寄存器定位</font>**<font style="color:rgb(64, 64, 64);">  
</font><font style="color:rgb(64, 64, 64);">进程切换时，CPU的CR3寄存器会更新为当前进程页目录的物理地址。通过CR3，MMU（内存管理单元）在地址转换时找到一级页表的位置。</font>
3. **<font style="color:rgb(64, 64, 64);">内核空间管理</font>**<font style="color:rgb(64, 64, 64);">  
</font><font style="color:rgb(64, 64, 64);">页目录属于内核数据结构，用户进程无法直接访问或修改。内核通过特权指令（如修改CR3）和内存管理算法维护这些页表。</font>

### **<font style="color:rgb(64, 64, 64);">二级页表（如页表项）</font>**
1. **<font style="color:rgb(64, 64, 64);">物理内存中</font>**<font style="color:rgb(64, 64, 64);">  
</font><font style="color:rgb(64, 64, 64);">二级页表（由一级页表条目指向的具体页表）同样存储在物理内存中。每个一级页表项（Page Directory Entry, PDE）指向一个二级页表的物理地址。</font>
2. **<font style="color:rgb(64, 64, 64);">动态分配与释放</font>**<font style="color:rgb(64, 64, 64);">  
</font><font style="color:rgb(64, 64, 64);">二级页表按需分配。当进程申请新内存时，内核可能动态创建对应的二级页表；当内存释放时，若页表不再被使用，内核会回收其物理内存。</font>
3. **<font style="color:rgb(64, 64, 64);">常驻内存优化</font>**<font style="color:rgb(64, 64, 64);">  
</font><font style="color:rgb(64, 64, 64);">页表通常常驻物理内存，避免因换页（swap）导致性能下降。但在极端内存压力下，某些系统可能将部分页表换出到磁盘，这种情况较为罕见。</font>

### **<font style="color:rgb(64, 64, 64);">关键点总结</font>**
+ **<font style="color:rgb(64, 64, 64);">存储位置</font>**<font style="color:rgb(64, 64, 64);">：一级和二级页表均位于物理内存，由内核直接管理。</font>
+ **<font style="color:rgb(64, 64, 64);">进程隔离性</font>**<font style="color:rgb(64, 64, 64);">：每个进程有独立的一级页表（页目录），通过CR3实现切换。</font>
+ **<font style="color:rgb(64, 64, 64);">用户无权限访问</font>**<font style="color:rgb(64, 64, 64);">：用户进程无法直接读写页表，必须通过内核的系统调用（如</font>`<font style="color:rgb(64, 64, 64);">mmap</font>`<font style="color:rgb(64, 64, 64);">、</font>`<font style="color:rgb(64, 64, 64);">brk</font>`<font style="color:rgb(64, 64, 64);">）间接修改映射关系。</font>
+ **<font style="color:rgb(64, 64, 64);">硬件依赖</font>**<font style="color:rgb(64, 64, 64);">：具体层级结构（如二级、三级甚至四级页表）因CPU架构而异（如x86、ARM）。</font>

### **<font style="color:rgb(64, 64, 64);">示例流程（x86两级分页）</font>**
1. <font style="color:rgb(64, 64, 64);">进程A运行时，CR3指向其页目录的物理地址。</font>
2. <font style="color:rgb(64, 64, 64);">MMU将虚拟地址拆解为页目录索引（一级）和页表索引（二级）。</font>
3. <font style="color:rgb(64, 64, 64);">通过页目录找到二级页表的物理地址，再通过页表找到最终物理页框。</font>
4. <font style="color:rgb(64, 64, 64);">若页表不存在，触发缺页异常，内核分配物理内存并更新页表。</font>

<font style="color:rgb(64, 64, 64);">总之，页表是内核维护的关键元数据，其物理存储和访问完全由操作系统控制，用户进程无感知</font>

:::

3. <font style="color:rgb(0, 0, 0);">在ARM32系统中，页表是如何映射的？在ARM64系统中，页表又是如何映射的？</font>

:::info
<font style="color:rgb(64, 64, 64);">在ARM32和ARM64系统中，页表映射机制存在显著差异，主要体现在地址空间划分、页表层级结构以及支持的物理地址扩展等方面。以下是两者的详细对比：</font>

:::

---

### **<font style="color:rgb(64, 64, 64);">ARM32系统的页表映射</font>**
1. **<font style="color:rgb(64, 64, 64);">基本结构</font>**
    - **<font style="color:rgb(64, 64, 64);">地址空间</font>**<font style="color:rgb(64, 64, 64);">：32位虚拟地址，物理地址通常为32位（LPAE扩展后支持40位）。</font>
    - **<font style="color:rgb(64, 64, 64);">页表层级</font>**<font style="color:rgb(64, 64, 64);">：</font>
        - [ ] **<font style="color:rgb(64, 64, 64);">普通模式（非LPAE）</font>**<font style="color:rgb(64, 64, 64);">：两级页表（页目录 + 页表项），支持段映射（1MB）和4KB小页。</font>
        * **<font style="color:rgb(64, 64, 64);">LPAE模式</font>**<font style="color:rgb(64, 64, 64);">：三级页表，支持物理地址扩展至40位，页大小可为4KB、2MB或1GB。</font>
2. **<font style="color:rgb(64, 64, 64);">地址划分（以LPAE为例）</font>**
    - <font style="color:rgb(64, 64, 64);">虚拟地址分为三级索引（每级9位）和页内偏移：</font>
        * **<font style="color:rgb(64, 64, 64);">Level 1索引</font>**<font style="color:rgb(64, 64, 64);">（9位）→</font><font style="color:rgb(64, 64, 64);"> </font>**<font style="color:rgb(64, 64, 64);">Level 2索引</font>**<font style="color:rgb(64, 64, 64);">（9位）→</font><font style="color:rgb(64, 64, 64);"> </font>**<font style="color:rgb(64, 64, 64);">Level 3索引</font>**<font style="color:rgb(64, 64, 64);">（9位）→</font><font style="color:rgb(64, 64, 64);"> </font>**<font style="color:rgb(64, 64, 64);">偏移量</font>**<font style="color:rgb(64, 64, 64);">（12位，4KB页）。</font>
    - <font style="color:rgb(64, 64, 64);">1GB大页通过Level 2表项直接映射，跳过Level 3。</font>
3. **<font style="color:rgb(64, 64, 64);">关键寄存器</font>**
    - `<font style="color:rgb(64, 64, 64);">TTBR0/TTBR1</font>`<font style="color:rgb(64, 64, 64);">：分别存储用户/内核空间页表基址（LPAE模式下使用</font>`<font style="color:rgb(64, 64, 64);">TTBR0</font>`<font style="color:rgb(64, 64, 64);">）。</font>
4. **<font style="color:rgb(64, 64, 64);">表项属性</font>**
    - <font style="color:rgb(64, 64, 64);">包含权限位（读/写）、缓存策略（如WB/WT）、安全域（NS位）和执行权限（XN位）。</font>

---

### **<font style="color:rgb(64, 64, 64);">ARM64系统的页表映射</font>**
1. **<font style="color:rgb(64, 64, 64);">基本结构</font>**
    - **<font style="color:rgb(64, 64, 64);">地址空间</font>**<font style="color:rgb(64, 64, 64);">：通常采用48位虚拟地址（可扩展至52位），物理地址支持48位。</font>
    - **<font style="color:rgb(64, 64, 64);">页表层级</font>**<font style="color:rgb(64, 64, 64);">：四级页表（4KB页时），支持灵活的页大小（4KB、16KB、64KB等）和大页（2MB、1GB）。</font>
2. **<font style="color:rgb(64, 64, 64);">地址划分（48位虚拟地址，4KB页）</font>**
    - <font style="color:rgb(64, 64, 64);">虚拟地址分为四级索引（每级9位）和页内偏移：</font>
        * **<font style="color:rgb(64, 64, 64);">Level 0索引</font>**<font style="color:rgb(64, 64, 64);">（9位）→</font><font style="color:rgb(64, 64, 64);"> </font>**<font style="color:rgb(64, 64, 64);">Level 1索引</font>**<font style="color:rgb(64, 64, 64);">（9位）→</font><font style="color:rgb(64, 64, 64);"> </font>**<font style="color:rgb(64, 64, 64);">Level 2索引</font>**<font style="color:rgb(64, 64, 64);">（9位）→</font><font style="color:rgb(64, 64, 64);"> </font>**<font style="color:rgb(64, 64, 64);">Level 3索引</font>**<font style="color:rgb(64, 64, 64);">（9位）→</font><font style="color:rgb(64, 64, 64);"> </font>**<font style="color:rgb(64, 64, 64);">偏移量</font>**<font style="color:rgb(64, 64, 64);">（12位）。</font>
    - <font style="color:rgb(64, 64, 64);">大页通过中间层级表项直接映射（如1GB大页由Level 1表项映射）。</font>
3. **<font style="color:rgb(64, 64, 64);">关键寄存器</font>**
    - `<font style="color:rgb(64, 64, 64);">TTBR0_EL1/TTBR1_EL1</font>`<font style="color:rgb(64, 64, 64);">：分别管理用户空间和内核空间的页表基址。</font>
4. **<font style="color:rgb(64, 64, 64);">表项属性</font>**
    - <font style="color:rgb(64, 64, 64);">64位表项，支持更多控制位，如权限（AP[2:0]）、内存类型（MAIR配置）、标记位（Contiguous/DBM）及执行权限（UXN/PXN）。</font>

:::info
### **<font style="color:rgb(64, 64, 64);">核心差异总结</font>**
:::

| **特性** | **ARM32** | **ARM64** |
| --- | --- | --- |
| **<font style="color:rgb(64, 64, 64);">地址空间</font>** | <font style="color:rgb(64, 64, 64);">32位虚拟，32/40位物理</font> | <font style="color:rgb(64, 64, 64);">48/52位虚拟，48位物理</font> |
| **<font style="color:rgb(64, 64, 64);">页表层级</font>** | <font style="color:rgb(64, 64, 64);">2级（普通模式）/3级（LPAE）</font> | <font style="color:rgb(64, 64, 64);">4级（默认）</font> |
| **<font style="color:rgb(64, 64, 64);">页大小支持</font>** | <font style="color:rgb(64, 64, 64);">4KB、64KB、1MB（段）</font> | <font style="color:rgb(64, 64, 64);">4KB、16KB、64KB、2MB、1GB</font> |
| **<font style="color:rgb(64, 64, 64);">表项大小</font>** | <font style="color:rgb(64, 64, 64);">4字节（非LPAE）/8字节（LPAE）</font> | <font style="color:rgb(64, 64, 64);">8字节</font> |
| **<font style="color:rgb(64, 64, 64);">物理扩展</font>** | <font style="color:rgb(64, 64, 64);">LPAE支持40位</font> | <font style="color:rgb(64, 64, 64);">原生支持48位</font> |
| **<font style="color:rgb(64, 64, 64);">权限控制</font>** | <font style="color:rgb(64, 64, 64);">较简单（XN位等）</font> | <font style="color:rgb(64, 64, 64);">更精细（UXN/PXN、AP[2:0]等）</font> |





4. <font style="color:rgb(0, 0, 0);">请简述Linux内核在理想情况下页面分配器(page allocator)是如何分配出连续物理页面的</font><font style="color:rgb(0, 0, 0);">。</font>

在理想情况下，Linux内核的页面分配器（也称为**伙伴系统**，Buddy Allocator）分配连续物理页面的过程可以简述为以下几点：

+ 初始化：系统启动时，页面分配器会将所有可用的物理内存页按照2的幂次方分组，形成不同大小的块。每个块内部的页面是连续的。
+ 查找空闲块：
    - 当有连续页面分配请求时，分配器首先根据请求的页面数量确定所需块的大小。
    - 然后从合适大小的空闲链表中查找是否有匹配的空闲块。
+ 合并与分割：
    - 如果找到的块过大，则将其分割成更小的块，直到满足请求的大小，并分配其中一部分给请求者。
    - 分配后剩余的小块会被重新加入到相应的空闲链表中。
    - 如果没有找到合适大小的块，分配器会尝试合并较小的空闲块来创建一个足够大的连续区域。
+ 分配页面：一旦找到或创建了合适的连续块，就将其标记为已使用并返回给请求者。
+ 释放页面：当不再需要这些页面时，它们会被标记为空闲，并尽可能地与其相邻的空闲块合并，以形成更大的空闲块供后续分配使用。

这种机制确保了在理想状态下能够高效地分配和回收连续的物理页面，同时尽量减少内存碎片



5. <font style="color:rgb(0, 0, 0);">在页面分配器中，如何从分配掩码(gfp_mask)中确定可以从哪些zone中分配内存？</font>

在 Linux 内核的页面分配器中，通过 gfp_mask（Get Free Page Mask）中的 区域修饰符（Zone Modifier）来确定允许从哪些内存区域（Zone）分配内存。具体流程如下：

1. <font style="color:rgb(0, 0, 0);">理解内存区域（Zone）划分  
</font><font style="color:rgb(0, 0, 0);">Linux 内核将物理内存划分为多个 zone，常见的 Zone 类型包括：</font>

<font style="color:rgb(0, 0, 0);">ZONE_DMA：供老式 DMA 设备使用的低地址内存（通常 < 16MB）。  
</font><font style="color:rgb(0, 0, 0);">ZONE_DMA32：供 64 位系统 DMA 设备使用的内存（通常 < 4GB）。  
</font><font style="color:rgb(0, 0, 0);">ZONE_NORMAL：直接映射到内核虚拟地址空间的常规内存（物理地址范围因架构而异）。  
</font><font style="color:rgb(0, 0, 0);">ZONE_HIGHMEM：高端内存（仅在 32 位系统中存在，用于映射超出内核虚拟地址空间的物理内存）。  
</font><font style="color:rgb(0, 0, 0);">2. 从 gfp_mask 提取 Zone 修饰符  
</font><font style="color:rgb(0, 0, 0);">gfp_mask 中通过以下标志位指定允许分配的内存区域：</font>

<font style="color:rgb(0, 0, 0);">__GFP_DMA：强制从 ZONE_DMA 分配。  
</font><font style="color:rgb(0, 0, 0);">__GFP_DMA32：强制从 ZONE_DMA32 分配。  
</font><font style="color:rgb(0, 0, 0);">__GFP_HIGHMEM：允许从 ZONE_HIGHMEM 分配（若存在）。  
</font><font style="color:rgb(0, 0, 0);">默认（无修饰符）：从 ZONE_NORMAL 或更低 Zone 分配。  
</font><font style="color:rgb(0, 0, 0);">3. 确定候选 Zone 列表  
</font><font style="color:rgb(0, 0, 0);">分配器根据 gfp_mask 的修饰符生成候选 Zone 列表，规则如下：</font>

<font style="color:rgb(0, 0, 0);">显式指定：若设置了 __GFP_DMA/__GFP_DMA32，则直接限定在对应 Zone。  
</font><font style="color:rgb(0, 0, 0);">隐式回退：若未指定修饰符，分配器按 Zone 优先级顺序（如 ZONE_NORMAL → ZONE_DMA32 → ZONE_DMA）尝试分配，优先选择更高地址的 Zone。  
</font><font style="color:rgb(0, 0, 0);">HIGHMEM 处理：仅在 __GFP_HIGHMEM 存在时允许使用 ZONE_HIGHMEM，但需内核支持。  
</font><font style="color:rgb(0, 0, 0);">4. 结合 Zone 水位检查  
</font><font style="color:rgb(0, 0, 0);">候选 Zone 还需满足 水位（Watermark）条件（如 min/low/high 阈值），分配器会：</font>

<font style="color:rgb(0, 0, 0);">检查候选 Zone 的空闲页面是否足够。  
</font><font style="color:rgb(0, 0, 0);">若当前 Zone 空闲页面不足，触发内存回收（如 kswapd）或尝试下一个候选 Zone。  
</font><font style="color:rgb(0, 0, 0);">5. 示例场景  
</font><font style="color:rgb(0, 0, 0);">GFP_KERNEL（默认修饰符）：  
</font><font style="color:rgb(0, 0, 0);">从 ZONE_NORMAL 开始分配，若失败则回退到 ZONE_DMA32 或 ZONE_DMA。  
</font><font style="color:rgb(0, 0, 0);">GFP_DMA：  
</font><font style="color:rgb(0, 0, 0);">强制从 ZONE_DMA 分配，仅用于需要低地址内存的设备驱动。  
</font><font style="color:rgb(0, 0, 0);">总结  
</font><font style="color:rgb(0, 0, 0);">通过解析 gfp_mask 中的 Zone 修饰符，页面分配器确定候选 Zone 列表，并按照优先级顺序尝试分配，同时结合水位检查确保内存分配的可行性。这一机制平衡了不同硬件需求（如 DMA）和内存使用效率。</font>



6. <font style="color:rgb(0, 0, 0);">页面分配器是按照什么方向来扫描zone的？</font>
+ 扫描方向：从高优先级 Zone 到低优先级 Zone（Fallback 顺序）。
+ 核心规则：
    - 默认按架构预设的 `zonelist` 顺序扫描。
    - `gfp_mask` 可显式限定允许的 Zone。
    - NUMA 系统优先本地节点内存。
+ 设计目标：在满足请求的前提下，尽可能保护低端 Zone（如 DMA 区域）不被过度消耗。

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

7. <font style="color:rgb(0, 0, 0);">为用户进程分配物理内存，分配掩码应该选用GFP_KERNEL，还是GFP_HIGHUSER_MOVABLE呢？</font>

在 Linux 内核中，为用户进程分配物理内存时，通常应选择 `GFP_HIGHUSER_MOVABLE` 而非 `GFP_KERNEL`。以下是详细分析和选择依据：

---

### 1. 分配掩码的核心区别
| 标志 | 适用场景 | 内存类型 | 可迁移性 | 潜在行为 |
| --- | --- | --- | --- | --- |
| `GFP_KERNEL` | 内核内部数据结构（如 slab 缓存） | 内核空间不可移动内存 | 不可迁移（`!__GFP_MOVABLE`<br/>） | 允许阻塞、触发直接内存回收 |
| `GFP_HIGHUSER_MOVABLE` | 用户进程内存（如堆、匿名页） | 用户空间可移动内存 | 可迁移（`__GFP_MOVABLE`<br/>） | 允许阻塞，但优先使用高端内存 |


---

### 2. 选择 `GFP_HIGHUSER_MOVABLE` 的原因
#### （1）用户内存与内核内存的隔离
+ 用户进程内存属于用户空间，应通过 `GFP_USER` 系列标志分配，以明确区分内存用途。
+ `GFP_KERNEL` 专用于内核内部内存（如驱动、文件系统缓存），若错误用于用户进程内存，可能导致：
    - 内核内存被用户空间占用，影响系统稳定性。
    - 内存回收时优先牺牲用户内存，违反预期行为。

#### （2）可迁移性优化
+ `GFP_HIGHUSER_MOVABLE` 包含 `__GFP_MOVABLE` 标志，允许内存管理子系统迁移物理页面以减少碎片。
+ 用户进程的堆、栈、匿名映射（如 `malloc` 分配的内存）通常无需固定物理地址，可迁移性提升内存利用率。

#### （3）NUMA 和高端内存适配
+ NUMA 优化：`GFP_HIGHUSER_MOVABLE` 隐含 `__GFP_THISNODE` 的 NUMA 本地性策略（取决于配置），优先从当前节点的内存分配。
+ 高端内存支持：在 32 位系统中，用户进程可能使用高端内存（`ZONE_HIGHMEM`），而 `GFP_HIGHUSER_MOVABLE` 允许从该区域分配。

---

### 3. 典型应用场景
#### （1）用户进程匿名页分配
用户进程通过 `mmap` 或 `malloc` 申请内存时，内核最终调用 `alloc_pages`，掩码应为：

```plain
c

gfp_t gfp_mask = GFP_HIGHUSER_MOVABLE | __GFP_ZERO;  // 可选清零内存
struct page *page = alloc_pages(gfp_mask, order);
```

#### （2）共享内存或文件映射
对于共享内存（`shmget`）或文件映射（`mmap` with file），掩码可能略有不同，但仍属于用户内存范畴：

```plain
c

gfp_t gfp_mask = GFP_HIGHUSER_MOVABLE | __GFP_WRITE; // 允许写入回写
```

---

### 4. `GFP_KERNEL` 的错误使用风险
若误用 `GFP_KERNEL` 分配用户进程内存：

+ 内存不可迁移：增加内存碎片，降低大块连续内存分配成功率。
+ 优先级倒置：内核内存回收可能优先压缩用户进程内存，导致性能下降。
+ 安全边界模糊：破坏用户空间与内核空间的内存隔离设计。

---

### 5. 例外情况
在以下特殊场景中，可能需要调整分配策略：

+ 硬件 DMA 内存：若用户进程内存需要用于 DMA 操作且设备不支持散射-聚集（Scatter-Gather），需使用 `GFP_DMA` 或 `GFP_DMA32`。
+ 内存锁定（mlock）：用户进程调用 `mlock` 锁定内存时，内核需分配不可移动内存（掩码中移除 `__GFP_MOVABLE`）。

---

### 6. 总结
+ 常规用户进程内存分配：始终优先使用 `GFP_HIGHUSER_MOVABLE`。
+ 内核内部内存分配：使用 `GFP_KERNEL`。
+ 特殊需求：根据硬件约束（如 DMA）或 API 要求（如 `mlock`）调整掩码

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

8. <font style="color:rgb(0, 0, 0);">slab分配器是如何分配和释放小块内存的？</font>

Slab分配器是Linux内核中用于高效管理小块内存的机制，特别适合频繁分配和释放的内核对象（如`task_struct`、`inode`等）。其核心思想是通过预分配和缓存对象来减少内存碎片及分配开销。以下是其分配和释放过程的详细说明：

---

### 一、Slab分配器的核心组件
1. 缓存（Cache）  
   每个缓存对应一种特定类型的内核对象（如`kmem_cache`结构），管理相同大小的内存块。缓存分为：
    - 专用缓存：为特定对象类型创建（如`task_struct`）。
    - 通用缓存：用于一般的小内存请求（如`kmalloc-32`、`kmalloc-64`等）。
2. Slab  
   每个Slab是连续的一页或多页内存（从伙伴系统分配），被划分为多个相同大小的对象。Slab有三种状态：
    - 满（Full）：所有对象已被分配。
    - 部分满（Partial）：部分对象空闲。
    - 空（Empty）：所有对象均未使用。
3. 空闲列表（Free List）  
每个Slab维护一个链表，记录可用的空闲对象。

---

### 二、内存分配流程
1. 请求分配对象  
当内核需要分配一个对象时（如`kmem_cache_alloc()`），Slab分配器执行以下步骤：
2. 查找空闲对象
    - 优先从Per-CPU缓存分配：  
    每个CPU有一个本地缓存（`kmem_cache_cpu`），直接从中获取对象，避免锁竞争。
    - Per-CPU缓存为空时：  
    从共享的Slab列表（部分满或空Slab）中批量迁移对象到Per-CPU缓存。
3. 分配新Slab（若必要）
    - 如果所有Slab均满，从伙伴系统申请新的内存页，创建新Slab并初始化对象。
    - 新Slab被标记为部分满，并添加到缓存链表中。
4. 返回对象地址  
最终从选中的Slab中取出一个空闲对象，更新空闲列表，并返回其虚拟地址。

---

### 三、内存释放流程
1. 请求释放对象  
调用`kmem_cache_free()`释放对象。
2. 对象返回到Per-CPU缓存
    - 优先将对象放回当前CPU的本地缓存，减少全局锁的使用。
    - 若Per-CPU缓存已满，批量将对象迁移到全局Slab的空闲列表。
3. Slab状态更新
    - 当某个Slab的所有对象均被释放时，标记为“空Slab”。
    - 内存回收策略：  
    若系统内存紧张，空Slab会被释放回伙伴系统（调用`__free_pages()`）。
4. 延迟释放优化  
Slab分配器可能延迟释放完全空闲的Slab，避免频繁申请/释放内存页的开销。

---

### 四、关键优化机制
1. Per-CPU缓存  
每个CPU维护本地对象缓存，减少多核竞争，提升分配速度。
2. Slab着色（Slab Coloring）  
通过调整Slab内部对象的偏移量，优化CPU缓存的利用率，减少缓存行冲突。
3. NUMA感知  
在NUMA架构中，优先从本地节点的内存分配Slab，降低远程访问延迟。
4. 调试与统计  
通过`/proc/slabinfo`查看各缓存的详细信息（如对象数量、内存使用等），支持内存泄漏检测。

---

### 五、Slab分配器的变种
1. SLUB（Unqueued Slab Allocator）
    - 简化了Slab的管理结构，减少元数据开销。
    - 默认用于现代Linux内核。
2. SLOB（Simple List Of Blocks）
    - 适用于嵌入式系统等内存受限环境。
    - 通过链表管理内存块，牺牲性能换取极低的内存开销。

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

9. <font style="color:rgb(0, 0, 0);">slab分配器中有一个着色的概念(cache color)，着色有什么作用？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

10. <font style="color:rgb(0, 0, 0);">slab分配其中的slab对象有没有根据Per-CPU做一些优化？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

11. <font style="color:rgb(0, 0, 0);">slab增长并导致大量不用的空闲对象，该如何解决？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

12. <font style="color:rgb(0, 0, 0);">请问kmalloc、vmalloc和malloc之间有什么区别以及实现上的差异？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

13. <font style="color:rgb(0, 0, 0);">使用用户态的API函数malloc()分配内存时，会马上为其分配物理内存吗？</font>

<font style="color:rgb(6, 6, 7);">在使用用户态的API函数 </font>`malloc()`<font style="color:rgb(6, 6, 7);"> 分配内存时，并不会立即为其分配物理内存。</font>`malloc()`<font style="color:rgb(6, 6, 7);"> 是一个标准的C库函数，它主要负责在堆上分配逻辑内存空间，并返回一个指向该空间的指针。这个过程主要涉及以下几个阶段：</font>

### <font style="color:rgb(6, 6, 7);">1.</font><font style="color:rgb(6, 6, 7);"> </font>**<font style="color:rgb(6, 6, 7);">分配虚拟内存空间</font>**
<font style="color:rgb(6, 6, 7);">当调用 </font>`malloc()`<font style="color:rgb(6, 6, 7);"> 时，它会通过底层的内存分配器（如 glibc 的 ptmalloc 或其他内存分配器）请求一块虚拟内存空间。这个空间在虚拟内存中被分配，但并不一定立即分配物理内存。</font>

### <font style="color:rgb(6, 6, 7);">2.</font><font style="color:rgb(6, 6, 7);"> </font>**<font style="color:rgb(6, 6, 7);">延迟分配物理内存</font>**
<font style="color:rgb(6, 6, 7);">在现代操作系统中，物理内存的分配通常是延迟的（Lazy Allocation）。这意味着当 </font>`malloc()`<font style="color:rgb(6, 6, 7);"> 分配虚拟内存后，操作系统并不会立即为其分配物理内存。只有当程序首次访问分配的内存（如读取或写入）时，才会触发一个页面错误（Page Fault），操作系统才会真正分配物理内存页，并将其映射到虚拟内存空间。</font>

<font style="color:rgb(6, 6, 7);">这种机制称为 </font>**<font style="color:rgb(6, 6, 7);">“按需分页”（Demand Paging）</font>**<font style="color:rgb(6, 6, 7);">，它允许操作系统更高效地管理物理内存，避免为未使用的内存分配物理资源。</font>

### <font style="color:rgb(6, 6, 7);">3.</font><font style="color:rgb(6, 6, 7);"> </font>**<font style="color:rgb(6, 6, 7);">特殊情况</font>**
<font style="color:rgb(6, 6, 7);">虽然物理内存的分配通常是延迟的，但有些情况下可能会立即分配物理内存：</font>

+ **<font style="color:rgb(6, 6, 7);">使用特定的分配策略</font>**<font style="color:rgb(6, 6, 7);">：某些内存分配器或操作系统可能会提供选项，强制立即分配物理内存。例如，Linux 中的 </font>`mlock()`<font style="color:rgb(6, 6, 7);"> 函数可以锁定内存，确保分配的内存立即分配物理页。</font>
+ **<font style="color:rgb(6, 6, 7);">内存初始化</font>**<font style="color:rgb(6, 6, 7);">：如果 </font>`malloc()`<font style="color:rgb(6, 6, 7);"> 返回的内存被初始化为特定值（如 </font>`calloc()`<font style="color:rgb(6, 6, 7);">），操作系统可能会立即分配物理内存以完成初始化。</font>

### <font style="color:rgb(6, 6, 7);">总结</font>
<font style="color:rgb(6, 6, 7);">在大多数情况下，</font>`malloc()`<font style="color:rgb(6, 6, 7);"> 分配内存时不会立即分配物理内存，而是分配虚拟内存空间。物理内存的分配通常是在首次访问分配的内存时按需进行的。这种机制有助于提高内存使用效率，避免浪费物理资源。</font>

<font style="color:rgb(6, 6, 7);"></font>

<font style="color:rgb(6, 6, 7);"></font>

<font style="color:rgb(6, 6, 7);"></font>

14. <font style="color:rgb(0, 0, 0);">假设不考虑libc的因素，malloc分配100Byte，那么实际上内核是为其分配100Byte吗？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

15. <font style="color:rgb(0, 0, 0);">假设两个用户进程打印的malloc()分配的虚拟地址是一样的，那么在内核中这两块虚拟内存是否打架了呢？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

16. <font style="color:rgb(0, 0, 0);">vm_normal_page()函数返回的是什么样页面的struct page数据结构？为什么内存管理代码中需要这个函数？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

17. <font style="color:rgb(0, 0, 0);">请简述get_user_page()函数的作用和实现流程？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

18. <font style="color:rgb(0, 0, 0);">请简述follow_page()函数的作用和实现流程？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

19. <font style="color:rgb(0, 0, 0);">请简述私有映射和共享映射的区别。</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

20. <font style="color:rgb(0, 0, 0);">为什么第二次调用mmap时，Linux内核没有捕捉到地址重叠并返回失败呢？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

21. <font style="color:rgb(0, 0, 0);">struct page数据结构中的_count和_mapcount有什么区别？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

22. <font style="color:rgb(0, 0, 0);">匿名页面和page cache页面有什么区别？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

23. <font style="color:rgb(0, 0, 0);">struct page数据结构中有一个锁，请问trylock_page()和lock_page()有什么区别？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

24. <font style="color:rgb(0, 0, 0);">在Linux 2.4.x内核中，如何从一个page找到所有映射该页面的VMA？反响映射可以带来哪些便利？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

25. <font style="color:rgb(0, 0, 0);">阅读Linux 4.0内核RMAP机制的代码，画出父子进程之间VMA、AVC、anon_vma和page等数据结构之间的关系图。</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

26. <font style="color:rgb(0, 0, 0);">在Linux 2.6.34中，RMAP机制采用了新的实现，在Linux 2.6.33和之前的版本中称为旧版本RMAP机制。那么在旧版本RMAP机制中，如果父进程有1000个子进程，每个子进程都有一个VMA，这个VMA里面有1000个匿名页面，当所有的子进程的VMA同时发生写复制时会是什么情况呢？</font>

<font style="color:rgb(0, 0, 0);"></font>

<font style="color:rgb(0, 0, 0);"></font>

27. <font style="color:rgb(0, 0, 0);">当page加入lru链表中，被其他线程释放了这个page，那么lru链表如何知道这个page已经被释放了。</font>
28. <font style="color:rgb(0, 0, 0);">kswapd内核线程何时会被唤醒？</font>
29. <font style="color:rgb(0, 0, 0);">LRU链表如何知道page的活动频繁程度？</font>
30. <font style="color:rgb(0, 0, 0);">kswapd按照什么原则来换出页面？</font>
31. <font style="color:rgb(0, 0, 0);">kswapd按照什么方向来扫描zone？</font>
32. <font style="color:rgb(0, 0, 0);">kswapd以什么标准来退出扫描LRU？</font>
33. <font style="color:rgb(0, 0, 0);">手持设备例如Android系统，没有swap分区或者swap文件，kswapd会扫描匿名页面LRU吗？</font>
34. <font style="color:rgb(0, 0, 0);">swappiness的含义是什么？kswapd如何计算匿名页面和page cache之间的扫描比重？</font>
35. <font style="color:rgb(0, 0, 0);">当系统充斥着大量只访问一次的文件访问(use-one streaming IO)时，kswapd如何来规避这种风暴？</font>
36. <font style="color:rgb(0, 0, 0);">在回收page cache时，对于dirty的page cache，kswapd会马上回写吗？</font>
37. <font style="color:rgb(0, 0, 0);">内核有哪些页面会被kswapd写回交换分区？</font>
38. <font style="color:rgb(0, 0, 0);">ARM32 Linux如何模拟这个Linux版本的L_PTE_YOUNG比特位呢？</font>
39. <font style="color:rgb(0, 0, 0);">如何理解Refault Distance算法？</font>
40. <font style="color:rgb(0, 0, 0);">请简述匿名页面的生命周期。在什么情况下会产生匿名页面？在什么条件下会释放匿名页面？</font>
41. <font style="color:rgb(0, 0, 0);">KSM是基于什么原理来合并页面的？</font>
42. <font style="color:rgb(0, 0, 0);">在KSM机制里，合并过程中把page设置成写保护的函数write_protect_page()有这样一个判断：。这个判断的依据是什么？</font>
43. <font style="color:rgb(0, 0, 0);">如果多个VMA的虚拟页面同时映射了同一个匿名页面，那么此时page->index应该等于多少？</font>
44. <font style="color:rgb(0, 0, 0);">为什么Dirty COW小程序可以修改一个只读文件的内容？</font>
45. <font style="color:rgb(0, 0, 0);">在Dirty COW内存漏洞中，如果Diryt COW程序没有madviseThread线程，即只有procselfmemThread线程，能否修改foo文件的内容呢？</font>
46. <font style="color:rgb(0, 0, 0);">假设在内核空间获取了某个文件对应的page cache页面的struct page数据结构，而对应的VMA属性是只读，那么内核空间是否可以成功修改该文件呢？</font>
47. <font style="color:rgb(0, 0, 0);">如果用户进程使用只读属性(PROT_READ)来mmap映射一个文件到用户空间，然后使用memcpy来写这段内存空间，会是什么样的情况？</font>
48. <font style="color:rgb(0, 0, 0);">请画出内存管理中常用的数据结构的关系图，如mm_struct、vma、vaddr、page、pfn、pte、zone、paddr和pg_data等，并思考如下转换关系。</font>
49. <font style="color:rgb(0, 0, 0);">请画出在最糟糕的情况下分配若干个连续物理页面的流程图。</font>
50. 在Android中新添加了LMK(Low Memory Killer)，请描述LMK和OOM Killer之间的关系。
51. <font style="color:rgb(0, 0, 0);">请描述一致性DMA映射dma_alloc_coherent()函数在AEM中是如何管理cache一致性的？</font>
52. <font style="color:rgb(0, 0, 0);">请描述流式DMA映射dma_map_single()函数在ARM中是如何管理cache一致性的？</font>
53. <font style="color:rgb(0, 0, 0);">为什么在Linux 4.8内核中要把基于zone的LRU链表机制迁移到基于Node呢？</font>

