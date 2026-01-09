#   1.设备树的引入与作用

​	为什么引入LED？前面platform总线，device和driver一起就把人搞得焦头烂额，这还只是驱动gpio控制led，如果需要更多的硬件设备，那不是需要写更多的device程序？非常不智能化，而且使得Linux越聚越多，所以Linux开始引入了设备树。

​	**设备树只是用来给内核里的驱动程序**，指定**硬件的信息**。比如 LED 驱动，在内核的驱动程序里去操作寄存器，但是操作哪一个引脚？这由设备树指定。

**总结：**

​	**设备树不是驱动；**

​	**设备树是：硬件资源的“说明书”；**

**为什么需要设备树？**

​	**同一个SOC（RK3566/IMX6ULL）；**

​	**不同的厂家，不同的开发板；**

​	**GPIO的接法不同；**

​	**I2C的外设不一样；**

​	**LED/屏幕/摄像头都不一样**

**内核不可能为每个开发板写一套代码。**

**设备树解决了什么？**

| 问题       | 没设备树      | 有设备树 |
| ---------- | ------------- | -------- |
| 硬件描述   | 写死在 C 代码 | DTS      |
| 板级差异   | 改内核        | 改 DTS   |
| 驱动复用   | 极差          | 极好     |
| 自制开发板 | 几乎不可能    | 可行     |

# 2.设备树语法介绍

​	上面讲述了为什么引入设备树，接下来学习设备树的语法；

如何编写设备树文件（dts：device tree source）；他需要被编译成dtb文件（dtb：device tree blob）

```dts
/dts-v1/;

#include "rk3566.dtsi"

/ {
    model = "My RK3566 Board";
    compatible = "myvendor,rk3566-myboard", "rockchip,rk3566";

    chosen {
        bootargs = "console=ttyFIQ0,1500000 root=/dev/mmcblk0p2 rw";
    };

    memory@0 {
        device_type = "memory";
        reg = <0x0 0x80000000>;
    };
};
```

上面时一个最小 DTS 的完整骨架。

```
/dts-v1/;     /*表示版本*/
#include "rk3566.dtsi" ：内核自带的 SoC 级设备树 include 文件
有些设备树文件还有
/dts-v1/;

#include <dt-bindings/input/input.h>
#include "imx6ull.dtsi"
#include <dt-bindings/input/input.h>：这个里面全是宏定义
/：根目录
```

## 2.1.DTS 的基本语法规则

**节点（Node）格式**：

```
label: node-name@unit-address {
    property = value;
    subnode {
        ...
    };
};
label           ← 可选，用来引用
node-name       ← 设备类型
@unit-address   ← 寄存器地址（可选）
示例（UART）：
uart2: serial@fe660000 {
    compatible = "rockchip,rk3566-uart";
    reg = <0xfe660000 0x100>;
    status = "okay";
};

```

**属性（Property）：**

```
1.字符串：
status = "okay";
model = "My Board";
数字（32 位 cell）：
clock-frequency = <24000000>;
数组 / 多 cell：
reg = <0xfe660000 0x100>; //起始地址 + 长度
布尔属性（没有值）：
dma-coherent; 		//有就代表 true
注释：
// 单行注释

/*
 * 多行注释
 */
```

## 2.2.引用机制（DTS 的“指针”）

**label + & 引用**：

```
&uart2 {
    status = "okay";
};
uart2: 是 label

&uart2 是引用
```

**phandle（自动生成的句柄）：**

```
gpios = <&gpio1 3 GPIO_ACTIVE_HIGH>;
```

| 内容               | 含义        |
| ------------------ | ----------- |
| `&gpio1`           | GPIO 控制器 |
| `3`                | 引脚号      |
| `GPIO_ACTIVE_HIGH` | 极性        |

# 3.重新编译内核文件

​	之前编译的正点原子的内核有点小问题，所以重新编译一下。

​	以下都是正点原子阿尔法开发板的编译步骤：

```sh
#需要先到内核文件夹里面，再进行一下操作
cd arch/arm/configs
cp imx_v7_mfg_defconfig imx_alientek_emmc_defconfig
#打开 imx_alientek_emmc_defconfig 文件，找到“CONFIG_ARCH_MULTI_V6=y”这一行，将其屏蔽掉
make imx_alientek_emmc_defconfig
cd arch/arm/boot/dts
cp imx6ull-14x14-evk.dts imx6ull-alientek-emmc.dts
#然后再imx6ull-alientek-emmc.dts的dtb-$(CONFIG_SOC_IMX6ULL)中添加imx6ull-alientek-emmc.dtb
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- distclean
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihfimx_alientek_emmc_defconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- menuconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-  HOSTCFLAGS="-fcommon" -j16
#需要添加：HOSTCFLAGS="-fcommon"；这是解决新版GCC和老版本的Linux内核的冲突，就是yylloc
```

给开发板更换dtb文件：

```
/*为了查看是否更换先添加一个节点*/
db_demo {
                led_demo = "dbs_led01";
        };
```

![](D:\学习笔记\驱动学习_重新开始\test6_device_tree\截图\修改的设备树信息.png)

​	将开发板中的dtb文件就是这个fdt文件拷贝会虚拟机中将其反编译为dts；

```sh
./scripts/dtc/dtc -I dtb -O dts -o tmp.dts ~/nfs_rootfs/imx6ull-alientek-emmc_fdt 
```

​	查看tmp.dts

```
db_demo {
                led_demo = "dbs_led01";
        };
```

​	就能找到我自己添加的节点。

​	以上就是更新设备树的方法了，不同的板子步骤都差不多了，我是使用tftp的形式传给uboot然后来启动开发板的，这一点正点原子的手册上有相关教程。

# 4.内核对设备树的处理

## 4.1.大致步骤

```
DTS-->DTB-->Device_node-->plotform_device
```

1. dts 在 PC 机上被编译为 dtb 文件；
2. u-boot 把 dtb 文件传给内核；
3. 内核解析 dtb 文件，把每一个节点都转换为 device_node 结构体；
4. 对于某些 device_node 结构体，会被转换为 platform_device 结构体。

## 4.2.哪些设备树节点会被转换为platform_device

1. 根节点下含有compatible属性的子节点

2. 含有特定compatible属性的节点的子节点

   1. 如果一个节点的 compatible属性，它的值是这 4 者之一："simple

      bus","simple-mfd","isa","arm,amba-bus", 那 么 它 的 子结点 ( 需 含

      compatible属性)也可以转换为 platform_device。

3. 总线 I2C、SPI 节点下的子节点：不转换为 platform_device

   某个总线下到子节点，应该交给对应的总线驱动程序来处理, 它们不应该被

   转换为 platform_device。

​	以上三点还有点混乱；我们需要理一下；什么是platform_bus

​	platform_bus：是 Linux 内核中的一种“虚拟总线类型”；

​	**platform_driver<-->platform_bus<-->platform_device**

​	**compatile是什么？compatible = “我是谁，能被谁驱动”**

```dts
/{
	usb{
		compatible = "myusb","simple_bus";
		usb0{
			compatible = "usb0";
		};
	};
	dev{
		compatible="mydev";
		devs{
			compatible="devs";
		};
	};
};

/*以上usb0和dev会创建device_platform；devs不会被创建*/
/*/：根节点下含有compatible会被创建device_platform；总线节点下面的子节点会被创建device_platform*/
```

## 4.3.platform_device和platform_driver如何配对

​	设备树转换得来的platform_device会被注册进内核里面，以后当我们每注册一个platform_driver的时候，他们就会两两确定是否能够匹配，如果能够匹配就会调用platform_driver的probe函数。

```c
static int platform_match(struct device *dev, struct device_driver *drv)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct platform_driver *pdrv = to_platform_driver(drv);

	/* When driver_override is set, only bind to the matching driver */
	if (pdev->driver_override)
		return !strcmp(pdev->driver_override, drv->name);

	/* Attempt an OF style match first */
	if (of_driver_match_device(dev, drv))
		return 1;

	/* Then try ACPI style match */
	if (acpi_driver_match_device(dev, drv))
		return 1;

	/* Then try to match against the id table */
	if (pdrv->id_table)
		return platform_match_id(pdrv->id_table, pdev) != NULL;

	/* fall-back to driver name match */
	return (strcmp(pdev->name, drv->name) == 0);
}
```

以下是platform_device和platform_driver的结构体：

```c
struct platform_device {
	const char	*name;
	int		id;
	bool		id_auto;
	struct device	dev;
	u32		num_resources;
	struct resource	*resource;

	const struct platform_device_id	*id_entry;
	char *driver_override; /* Driver name to force a match */

	/* MFD cell pointer */
	struct mfd_cell *mfd_cell;

	/* arch specific additions */
	struct pdev_archdata	archdata;
};
```

```c
struct platform_driver {
	int (*probe)(struct platform_device *);
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	int (*suspend)(struct platform_device *, pm_message_t state);
	int (*resume)(struct platform_device *);
	struct device_driver driver;
	const struct platform_device_id *id_table;
	bool prevent_deferred_probe;
};

struct device_driver {
	const char		*name;
	struct bus_type		*bus;

	struct module		*owner;
	const char		*mod_name;	/* used for built-in modules */

	bool suppress_bind_attrs;	/* disables bind/unbind via sysfs */

	const struct of_device_id	*of_match_table;
	const struct acpi_device_id	*acpi_match_table;

	int (*probe) (struct device *dev);
	int (*remove) (struct device *dev);
	void (*shutdown) (struct device *dev);
	int (*suspend) (struct device *dev, pm_message_t state);
	int (*resume) (struct device *dev);
	const struct attribute_group **groups;

	const struct dev_pm_ops *pm;

	struct driver_private *p;
};
```

```
1️⃣ driver_override（最高优先级）这个看是否强制选择某个driver
2️⃣ 设备树 OF 匹配
3️⃣ ACPI 匹配
4️⃣ id_table 匹配
5️⃣ name 字符串匹配（兜底）
```

## 4.4**没有转换为** **platform_device** **的节点，如何使用**

```dts
leds {
    compatible = "gpio-leds";

    led0 {
        label = "red";
        gpios = <&gpio1 3 GPIO_ACTIVE_HIGH>;
    };
};
```

| 节点 | platform_device |
| ---- | --------------- |
| leds | ✅               |
| led0 | ❌               |

驱动中这样写：

```c
for_each_child_of_node(np, child) {
    of_property_read_string(child, "label", &name);
    of_get_named_gpio(child, "gpios", 0);
}
```

 **led0 是“数据节点”，不是设备**

​	这中of_xxx函数就是driver与设备树交互的函数

韦东山这里总结了各个头文件中包含的信息：

**1.处理DTB：**

```
of_fdt.h // dtb 文件的相关操作函数, 我们一般用不到, 
// 因为 dtb 文件在内核中已经被转换为 device_node 树(它更易于使用)
```

**2.处理device_node:**

```
of.h // 提供设备树的一般处理函数, 
// 比如 of_property_read_u32(读取某个属性的 u32 值),
// of_get_child_count(获取某个 device_node 的子节点数)
of_address.h // 地址相关的函数, 
// 比如 of_get_address(获得 reg 属性中的 addr, size 值)
// of_match_device (从 matches 数组中取出与当前设备最匹配的一项)
of_dma.h // 设备树中 DMA 相关属性的函数
of_gpio.h // GPIO 相关的函数
of_graph.h // GPU 相关驱动中用到的函数, 从设备树中获得 GPU 信息
of_iommu.h // 很少用到
of_irq.h // 中断相关的函数
of_mdio.h // MDIO (Ethernet PHY) API
of_net.h // OF helpers for network devices. 
of_pci.h // PCI 相关函数
of_pdt.h // 很少用到
of_reserved_mem.h // reserved_mem 的相关函数
```

**3.处理platform_device:**

```
of_platform.h // 把 device_node 转换为 platform_device 时用到的函数, 
 // 比如 of_device_alloc(根据 device_node 分配设置 platform_device), 
 // of_find_device_by_node (根据 device_node 查找到 platform_device),
 // of_platform_bus_probe (处理 device_node 及它的子节点)
of_device.h // 设备相关的函数, 比如 of_match_device
```

## 4.5.platform_device 相关的函数

### 4.5.1.会被转换为platform_device 节点的访问

​	以上将来platform_driver和device 的匹配，下面讲一下platform_device 的相关函数

**1.of_find_device_by_node：**

```c
struct platform_device *of_find_device_by_node(struct device_node *np)
{
	struct device *dev;

	dev = bus_find_device(&platform_bus_type, NULL, np, of_dev_node_match);
	return dev ? to_platform_device(dev) : NULL;
}
EXPORT_SYMBOL(of_find_device_by_node);
/*设备树中的每一个节点，在内核里都有一个 device_node；可以使用
device_node 去找到对应的 platform_device。*/
```

**2.platform_get_resource**

```c
struct resource *platform_get_resource(struct platform_device *dev,
				       unsigned int type, unsigned int num)
{
	int i;

	for (i = 0; i < dev->num_resources; i++) {
		struct resource *r = &dev->resource[i];

		if (type == resource_type(r) && num-- == 0)
			return r;
	}
	return NULL;
}
```

​	这 个 函 数 跟 设 备 树 没 什 么 关 系 ， 但 是 设 备 树 中 的 节 点 被 转 换 为platform_device 后，设备树中的 reg 属性、interrupts 属性也会被转换为“resource”。这时，就可以使用这个函数取出这些资源。

​	对于设备树节点中的 reg 属性，它对应 IORESOURCE_MEM 类型的资源；

​	对于设备树节点中的 interrupts 属性，它对应 IORESOURCE_IRQ 类型的资源。

### 4.5.2.**有些节点不会生成** platform_device，怎么访问它们

​	内核会把 dtb 文件解析出一系列的 device_node 结构体，我们可以直接访问这些 device_node。

​	内核源码 incldue/linux/of.h 中声明了 device_node 和属性 property的操作函数，device_node 和 property 的结构体定义如下：

```c
struct device_node {
	const char *name;
	const char *type;
	phandle phandle;
	const char *full_name;
	struct fwnode_handle fwnode;

	struct	property *properties;
	struct	property *deadprops;	/* removed properties */
	struct	device_node *parent;
	struct	device_node *child;
	struct	device_node *sibling;
	struct	kobject kobj;
	unsigned long _flags;
	void	*data;
#if defined(CONFIG_SPARC)
	const char *path_component_name;
	unsigned int unique_id;
	struct of_irq_controller *irq_trans;
#endif
};
```

```c
struct property {
	char	*name;
	int	length;
	void	*value;
	struct property *next;
	unsigned long _flags;
	unsigned int unique_id;
	struct bin_attribute attr;
};
```

1.of_find_node_by_path：

```c
static inline struct device_node *of_find_node_by_path(const char *path)
{
	return of_find_node_opts_by_path(path, NULL);
}
/*根据路径找到节点，比如“/”就对应根节点，“/memory”对应 memory 节点。*/
```

2.of_find_node_by_name

```c
extern struct device_node *of_find_node_by_name(struct device_node *from,
	const char *name);
/*根据名字找到节点，节点如果定义了 name 属性，那我们可以根据名字找到它。*/
//参数 from 表示从哪一个节点开始寻找，传入 NULL 表示从根节点开始寻找。
//但是在设备树的官方规范中不建议使用“name”属性，所以这函数也不建议使用。
```

3.of_find_node_by_type：

```c
extern struct device_node *of_find_node_by_type(struct device_node *from,
	const char *type);
/*根据类型找到节点，节点如果定义了 device_type 属性，那我们可以根据类型找到它。*/
/*
参数 from 表示从哪一个节点开始寻找，传入 NULL 表示从根节点开始寻找。
但是在设备树的官方规范中不建议使用“device_type”属性，所以这函数也不建议使用。
*/
```

4.of_find_compatible_node：

```c
extern struct device_node *of_find_compatible_node(struct device_node *from,
	const char *type, const char *compat);
/*根据 compatible 找到节点，节点如果定义了 compatible 属性，那我们可
以根据 compatible 属性找到它。*/
/*
参数 from 表示从哪一个节点开始寻找，传入 NULL 表示从根节点开始寻找。
参数 compat 是一个字符串，用来指定 compatible 属性的值；
参数 type 是一个字符串，用来指定 device_type 属性的值，可以传入 NULL。
*/
```

5.of_find_node_by_phandle

```c
extern struct device_node *of_find_node_by_phandle(phandle handle);
/*
根据 phandle 找到节点。dts 文件被编译为 dtb 文件时，每一个节点都有一个数字 ID，这些数字 ID 彼此不同。可以使用数字 ID 来找到 device_node。这些数字 ID 就是 phandle。
*/
```

6.of_get_parent：找到 device_node 的父节点。

```c
extern struct device_node *of_get_parent(const struct device_node *node);
```

7.of_get_next_parent：

```c
extern struct device_node *of_get_next_parent(struct device_node *node);
/*
这个函数名比较奇怪，怎么可能有“next parent”？
它实际上也是找到 device_node 的父节点，跟 of_get_parent 的返回结果是一样的。
差别在于它多调用下列函数，把 node 节点的引用计数减少了 1。这意味着调用 of_get_next_parent 之后，你不再需要调用 of_node_put 释放 node 节点。
*/
of_node_put(node);
extern struct device_node *of_get_next_parent(struct device_node *node);
```

8.of_get_next_child：取出下一个子节点。

```c
extern struct device_node *of_get_next_child(const struct device_node *node,
					     struct device_node *prev);
/*
参数 node 表示父节点；
prev 表示上一个子节点，设为 NULL 时表示想找到第 1 个子节点。
不断调用 of_get_next_child 时，不断更新 pre 参数，就可以得到所有的子节点。
*/
```

9.of_get_next_available_child：

```c
/*
取出下一个“可用”的子节点，有些节点的 status 是“disabled”，那就会跳过这些节点
*/
extern struct device_node *of_get_next_available_child(
	const struct device_node *node, struct device_node *prev);
```

10.of_get_child_by_name：根据名字找到子节点

```c
extern struct device_node *of_get_child_by_name(const struct device_node *node,
const char *name);
```

**找到属性——of_find_property**

**内核源码 incldue/linux/of.h 中声明了 device_node 的操作函数，当然也包括属性的操作函数：of_find_property**

```c
extern struct property *of_find_property(const struct device_node *np,
					 const char *name,
					 int *lenp);
/*
参数 np 表示节点，我们要在这个节点中找到名为 name 的属性。
lenp 用来保存这个属性的长度，即它的值的长度。
*/
/*
在设备树中，节点大概是这样：

xxx_node {
 xxx_pp_name = “hello”;
};
上述节点中，“xxx_pp_name”就是属性的名字，值的长度是 6。
*/
```

**获取属性的值**：

1.of_get_property：根据名字找到节点的属性，并且返回它的值。

```c
extern const void *of_get_property(const struct device_node *node,
				const char *name,
				int *lenp);
/*
参数 np 表示节点，我们要在这个节点中找到名为 name 的属性，然后返回它的值。
lenp 用来保存这个属性的长度，即它的值的长度。
*/
```

2.of_property_count_elems_of_size：根据名字找到节点的属性，确定它的值有多少个元素(elem)

```c
extern int of_property_count_elems_of_size(const struct device_node *np,
				const char *propname, int elem_size);
```

还有很多函数就不一一列举了，要用的时候就会了。

# 5.阅读imx6ull.dtsi

```dts
gpio1: gpio@0209c000 {
				compatible = "fsl,imx6ul-gpio", "fsl,imx35-gpio";
				reg = <0x0209c000 0x4000>;
				interrupts = <GIC_SPI 66 IRQ_TYPE_LEVEL_HIGH>,
					     <GIC_SPI 67 IRQ_TYPE_LEVEL_HIGH>;
				gpio-controller;
				#gpio-cells = <2>;
				interrupt-controller;
				#interrupt-cells = <2>;
			};
```

下面分析一下这段dts的代码：

```dts
reg = <0x0209c000 0x4000>;
```

| 字段       | 含义                                 |
| ---------- | ------------------------------------ |
| 0x0209c000 | **物理起始地址（Physical Address）** |
| 0x4000     | **占用空间大小（16KB）**             |

也就是说`0x0209c000 ~ 0x0209FFFF`都是`GPIO1`的寄存器空间

```
GPIO1 基地址 = 0x0209C000

0x0209C000  DR     数据寄存器
0x0209C004  GDIR   方向寄存器
0x0209C008  PSR    状态寄存器
0x0209C00C  ICR1
0x0209C010  ICR2
...
```

如何写：

```dts
/{
	myled1{
	compatible="db,db_led1";
	pinctrl-names="default";
	pinctrl-0=<&pinctrl_led>;
	led-gpio = <&gpio1 3 GPIO_ACTIVE_LOW>
	status = "okay";
	};
};
/*这里led-gpio = <&gpio1 3 GPIO_ACTIVE_LOW>为什么这样写？*/
/*在gpio.txt/gpio.yaml里面有写：
gpio-list ::= <single-gpio> [gpio-list]
single-gpio ::= <gpio-phandle> <gpio-specifier>
gpio-phandle : phandle to gpio controller node
gpio-specifier : Array of #gpio-cells specifying specific gpio

single-gpio ::= <gpio-phandle> <gpio-specifier>这一行就是我们写的：
	led-gpio = <&gpio1 3 GPIO_ACTIVE_LOW>
	gpio-phandle：这个一般被命名在.dtsi中也就是最上面的gpio1: gpio@0209c000
	<gpio-specifier>：由gpio1中的#gpio-cells = <2>;来决定意味着需要两个参数
*/
/*光有gpio还不够，我们还需要设置电气属性以及复用为gpio*/
&iomuxc {
	pinctrl_led: ledgrp 
	{ 
		fsl,pins = < 
			MX6UL_PAD_GPIO1_IO03__GPIO1_IO03 0x10b0 >; 
			/*MX6UL_PAD_GPIO1_IO03__GPIO1_IO03这里就是复用为gpio1；0x10b0是配置电气属性*/
	}; 
};
/*
且iomuxc要在根节点之外
因为：
    是对已有节点的引用与修改：
    iomuxc 节点已经在 SoC 的 .dtsi 里定义好了
    你这里只是 追加 pinctrl 配置
    不能重新定义成一个普通子节点
    这是 DTS/DTSI 的“overlay / patch”机制
*/
```

```c
/*伪代码*/
/*
1.注册主设备号：register_chrdev
2.根据主设备号注册时的需要封装：file_operations
3.创建/sys/xxx_class：class_create
4.由于device是由platform_device提供的所以device_create需要封装为框架层，并提供给外界使用（EXPORT_SYMBOL）
5.有创建就有led_class_destroy_device，这个也疯转为外部可用
6.如何控制led，设计led_operations
7.由于框架层不做任何操作led_operations的完成方法由外部提供，所以需要封装一个register_led_operations供外部使用
8.以上六步完成以后led_drv复用性大大提高，维护的时候基本不需要进行修改
9.实现led_operations，.init,.ctl;
10.platform_driver:name,probe,remove;name用做匹配，probe用作device_create的创建，remove就是移除了 然后platform_driver_register10.platform_device：name，start，end，num_resources，resource，.dev:release,然后platform_device_register
*/
```

这是上一节的伪代码，这一讲需要将其刨除掉platform_device的.c文件，这一部分是由设备树来写了。

# 6.代码演示

## 6.1.设备树代码

```
/{
	myled{
        compatible = "db,db_led1";
        pinctrl-names = "default";
                pinctrl-0 = <&pinctrl_led>;
        
        led-gpios = <&gpio1 3 GPIO_ACTIVE_LOW>;
        status = "okay";
    };

};
&iomuxc {
	pinctrl_led: ledgrp 
	{ 
		fsl,pins = < 
            MX6UL_PAD_GPIO1_IO03__GPIO1_IO03 0x10b0 
        >;
	};
};
```

​	这个需要注意在正点原子的设备树里面GPIO1_IO03这个pin脚已经被占用了，正点原子把他作为心跳的信号灯，需要给他注释掉。&iomuxc这个正点原子也写了一个，我们的pinctrl_led配置需要将他写入到正点原子的&iomuxc，不然无法识别到。也是同一个PAD被多次使用的结果。

​	然后led_drv.c没有变化，platform_device没有了,所以board_A_demo.c就不用写了，led_resouce.h就不用写了，只有chip_demo_gpio.c有变化：

```c
static const struct of_device_id db_leds[] = {
    {.compatible="db,db_led1"},
    {}
};
MODULE_DEVICE_TABLE(of, db_leds);
static struct platform_driver chip_demo_gpio = {
    .probe      =       chip_demo_gpio_probe,
    .remove     =       chip_demo_gpio_remove,
    .driver     =       {
        .name   =       "db_leds",
        .of_match_table =   db_leds,
    }
};
```

​	这一部分就是完全仿造韦东山写的代码；我的设备树里面compatible = "db,db_led1";所以db_leds数组里面也需要写{.compatible="db,db_led1"},接下来就是chip_demo_gpio_probe和chip_demo_gpio_remove函数有重大变化；

```c
struct board_led {
    struct gpio_desc *gpiod;
};
struct chip_gpio_priv {
    struct board_led *leds;
    int led_count;
};

static struct chip_gpio_priv *g_priv;
static int chip_demo_gpio_probe(struct platform_device *pdev)
{
    int i;
    /* 1.拿到device_node */
    struct device *dev = &pdev->dev;
    struct chip_gpio_priv *priv;
    //struct device_node *np = dev->of_node;
    
    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;
    /* 2.数一数有几个led */
    /*int gpiod_count(struct device *dev, const char *con_id) */
    priv->led_count = gpiod_count(dev, "led");
    if (priv->led_count < 0)
        return priv->led_count;
    /* 3.申请数组保存每一个LED的gpio_desc */
    priv->leds = devm_kzalloc(dev,
        sizeof(struct board_led) * priv->led_count,
        GFP_KERNEL);
    if (!priv->leds)
        return -ENOMEM;
    /* 4.逐个解析 led-gpios */
    for (i = 0; i < priv->led_count; i++) {
        priv->leds[i].gpiod = devm_gpiod_get_index(dev,
                            "led", i, GPIOD_OUT_LOW);
        if (IS_ERR(priv->leds[i].gpiod))
            return PTR_ERR(priv->leds[i].gpiod);
        led_class_device_create(i);
    }
    /* 5.把 leds / led_count 保存起来 */
    platform_set_drvdata(pdev, priv);
    g_priv = priv;
    register_led_operations(&board_demo_led_opr);
    return 0;
}

static int board_demo_led_init (int which) /* 初始化LED, which-哪个LED */       
{   
    if (!g_priv || which >= g_priv->led_count)
        return -EINVAL;

    /* 设置为输出，默认灭 */
    gpiod_direction_output(g_priv->leds[which].gpiod, 0);
    return 0;
}
```

​	以上的写法就是查资料的韦东山的课程还没有看，这里就涉及到Linux的gpio子系统。

## 6.2.驱动代码分析

```c
static const struct of_device_id db_leds[] = {
    {.compatible="db,db_led1"},
    {}
};
MODULE_DEVICE_TABLE(of, db_leds);
static struct platform_driver chip_demo_gpio = {
    .probe      =       chip_demo_gpio_probe,
    .remove     =       chip_demo_gpio_remove,
    .driver     =       {
        .name   =       "db_leds",
        .of_match_table =   db_leds,
    }
};
```

​	以上两个struct就是platform_driver非常重要的内容了，可以说是device和driver匹配的核心点；一般是写of_match_table与设备树中的compatible进行匹配；name就是上文说到的托底的作用了。而在上文我写到的chip_demo_gpio_probe和chip_demo_gpio_remove就是platform_driver需要实现的重点了。

​	大家可以发现不论是设备树还是driver代码中都没有再使用到虚拟地址的映射了，在教程中我们会学习到在probe函数中去进行寄存器地址的映射，这一点一直困惑我很久，因为写代码的时候一直没有进行寄存器地址映射，我感觉很奇怪，接下来看一小段代码：

```c
static int mxc_gpio_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct mxc_gpio_port *port;
	struct resource *iores;
	int irq_base;
	int err;

	mxc_gpio_get_hw(pdev);

	port = devm_kzalloc(&pdev->dev, sizeof(*port), GFP_KERNEL);
	if (!port)
		return -ENOMEM;

	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	port->base = devm_ioremap_resource(&pdev->dev, iores);
	if (IS_ERR(port->base))
		return PTR_ERR(port->base);

	port->irq_high = platform_get_irq(pdev, 1);
	port->irq = platform_get_irq(pdev, 0);
	if (port->irq < 0)
		return port->irq;
	................................
}

static const struct of_device_id mxc_gpio_dt_ids[] = {
	{ .compatible = "fsl,imx1-gpio", .data = &mxc_gpio_devtype[IMX1_GPIO], },
	{ .compatible = "fsl,imx21-gpio", .data = &mxc_gpio_devtype[IMX21_GPIO], },
	{ .compatible = "fsl,imx31-gpio", .data = &mxc_gpio_devtype[IMX31_GPIO], },
	{ .compatible = "fsl,imx35-gpio", .data = &mxc_gpio_devtype[IMX35_GPIO], },
	{ /* sentinel */ }
};
```

​	这个就是imx的gpio控制器驱动代码节选；devm_ioremap_resource从这个函数可以看出probe里面是做了寄存器映射的，为什么我们没有做？因为**功能性驱动（consumer driver）一般不需要、也不应该自己做寄存器映射。**因为我们只需要点灯，操作马达，或者IIC外设这一类的东西，这一套官方已经把子系统写好了，并给我们预留了接口，我们只需要在设备树中**描述设备与控制器的关系**，并在驱动里使用对应子系统的 API。然后**由子系统匹配；由子系统调度；由子系统转发到真实寄存器**。什么时候需要进行地址映射？手写控制器驱动（几乎不可能）；或者厂家给了一个奇怪的外设；没有gpio，i2c，spi，pwm，那我们就只能去进行寄存器映射然后做想要做的功能。

​	通过上面的总结就能看出内核的本质：内核是少量真正的“裸机级硬件代码”（控制器驱动），大量“抽象层 / 调度层 / 资源管理层和明确的分层与所有权规则组成的。

​	struct gpio_desc结构体：这个可以把他当作一个文件句柄。

```c
/*详细分析一下代码*/
	/* 1.拿到device_node */
    struct device *dev = &pdev->dev;
    struct chip_gpio_priv *priv;
    //struct device_node *np = dev->of_node;
    /*这一步是将dev挂载到我们创建的空间上*/
	/*devm_kzalloc函数非常好用，他会帮我们自动清零和自动释放*/
	/*devm_kzalloc函数和智能指针很像，但是devm_kzalloc进行释放是和设备生命周期有关，智能指针是作用域结束*/
    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;
```

```c
/* 2.数一数有几个led */
    /*int gpiod_count(struct device *dev, const char *con_id) */
    priv->led_count = gpiod_count(dev, "led");
    if (priv->led_count < 0)
        return priv->led_count;
/*这里就是计数了，用于创建多少了次设备*/
/*
在设备树中可以加设备数量
led-gpios = <&gpio1 3 GPIO_ACTIVE_LOW> 
			<&gpio2 5 GPIO_ACTIVE_LOW> ;
这里priv->led_count就会有两个计数
*/
```

```c
	 /* 3.申请数组保存每一个LED的gpio_desc */
    priv->leds = devm_kzalloc(dev,
        sizeof(struct board_led) * priv->led_count,
        GFP_KERNEL);
    if (!priv->leds)
        return -ENOMEM;
    /* 4.逐个解析 led-gpios */
    for (i = 0; i < priv->led_count; i++) {
        priv->leds[i].gpiod = devm_gpiod_get_index(dev,
                            "led", i, GPIOD_OUT_LOW);
        if (IS_ERR(priv->leds[i].gpiod))
            return PTR_ERR(priv->leds[i].gpiod);
        led_class_device_create(i);
/*这一段就非常明显的知道在干嘛了*/
```

## 6.3.如何拿到GPIO

​	这里讲一下我们的driver中如何拿到gpio的pin脚；

| 写法     | DT 属性     | 驱动拿法                   | 是否推荐 |
| -------- | ----------- | -------------------------- | -------- |
| **现代** | `led-gpios` | `devm_gpiod_get()`         | ⭐⭐⭐⭐⭐    |
| 兼容     | `led-gpio`  | `gpiod_get_from_of_node()` | ⭐⭐⭐⭐     |
| 老式     | `led-gpio`  | `of_get_named_gpio()`      | ⭐        |

### 6.3.1.方法一：devm_gpiod_get()

​	方法一完全就是gpio子系统的方法了，我们需要使用到`gpiolib`，`gpiolib`识别设备树的标准写法就是:
`xxx-gpios`；然后我们就可以使用**devm_gpiod_get(dev, "led", ...)**	或者
**devm_gpiod_get_index(dev, "led", i, ...)**

```c
for (i = 0; i < priv->led_count; i++) {
        priv->leds[i].gpiod = devm_gpiod_get_index(dev,
                            "led", i, GPIOD_OUT_LOW);
        if (IS_ERR(priv->leds[i].gpiod))
            return PTR_ERR(priv->leds[i].gpiod);
        led_class_device_create(i);
    }
```

如上我就使用到了devm_gpiod_get_index去拿取信息。

### 6.3.2.方法二：`gpiod_get_from_of_node()`

​	为什么使用这个方法，多是因为设备树中没有按gpio子系统的标准写法如：led-gpio = <>;

​	`led-gpio`（单数）不是 gpiolib 的标准 consumer 命名，内核不会自动帮你生成 `struct gpio_desc *`，需要自己从 `device_node` 里解析，再“手动”转换成 GPIO 或 gpiod。

```c
struct device_node *np = dev->of_node;
struct gpio_desc *desc;
enum of_gpio_flags flags;

desc = gpiod_get_from_of_node(
        np,            /* device_node */
        "led-gpio",    /* 属性名 */
        0,             /* index */
        GPIOD_OUT_LOW, /* dflags */
        "led");
if (IS_ERR(desc))
    return PTR_ERR(desc);
```

​	这种方法用于兼容一些旧DT，这是历史遗留性问题。

### 6.3.3.方法三：`of_get_named_gpio()`

```c
int gpio;
enum of_gpio_flags flags;
struct device_node *np = dev->of_node;
gpio = of_get_named_gpio_flags(np, "led-gpio", 0, &flags);
if (!gpio_is_valid(gpio))
    return -EINVAL;
/*然后再申请gpio*/
devm_gpio_request_one(dev, gpio, GPIOF_OUT_INIT_LOW, "led");
```

​	这种方式极其不推荐，不稳定；可以直接忽略。

# 7.总结

​	通过以上的学习，已经充分了解了设备树代码格式，如何写设备树文件，修改设备树文件的位置，驱动如何编写，如何操做gpio，已经有了一个大致的理解雏形，并且已经成功点亮和熄灭led。
