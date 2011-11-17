/**********************************文件头部注释************************************/
//
//
//                      Copyright (C), 2005-2010, AV Frontier Tech. Co., Ltd.
//
//
// 文件名：     ywgpio_def.h
//
// 创建者：     CS
//
// 创建时间：   2008.04.23
//
// 文件描述：   GPIO口功能定义的头文件。
//
// 修改记录：   日       期      作      者       版本      修定
//              ---------         ---------       -----     -----
//             2008.04.23         CS              0.01       新建
//
/*****************************************************************************************/


#ifndef __YWGPIO_DEF_H
#define __YWGPIO_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

/*
#define PIO_BIT_0    1
#define PIO_BIT_1    2
#define PIO_BIT_2    4
#define PIO_BIT_3    8
#define PIO_BIT_4    16
#define PIO_BIT_5    32
#define PIO_BIT_6    64
#define PIO_BIT_7    128
*/

#define        PIO_BITS                8


#ifdef CONFIG_CPU_SUBTYPE_STB7100
/******************************  PIO_0 引脚用法定义*********************************/

#define GPIO_SC0UART_TXD_PORT              0                /*SMART0 Uart Data TXD IO Port*/
#define GPIO_SC0UART_TXD_BIT               0

#define GPIO_SC0UART_RXD_PORT              0                /*SMART0 Uart Data RXD IO Port*/
#define GPIO_SC0UART_RXD_BIT               1

//cs added for NEW Grass Board.20091120
#ifdef GMOON_IBOX_VER1_0
#define GPIO_SMART_RESET_PORT              (0)              /*SMART CARD RESET*/
#define GPIO_SMART_RESET_BIT               (2)              /*用于板载CA 复位 ， 0 有效*/

#define GPIO_CA_SW_PORT                    (0)              /*CA 数据切换 ，1 为外部智能卡，0 为内部板载CA*/
#define GPIO_CA_SW_BIT                     (6)
#endif

#ifdef GRASS_IBOXHD9_VER1_1
#define GPIO_VCC_EN3_PORT                  0                /*CI卡座供电*/
#define GPIO_VCC_EN3_BIT                   2
#endif


//end add

#define GPIO_SC0_CLK_PORT                  0                /*SMART0 CLK IO Port*/
#define GPIO_SC0_CLK_BIT                   3

#define GPIO_SC0_RST_PORT                  0                /*SMART0 RST IO Port*/
#define GPIO_SC0_RST_BIT                   4

#define GPIO_SC0_VCC_PORT                  0                /*SMART0 VCC IO Port*/
#define GPIO_SC0_VCC_BIT                   5

#define GPIO_SC0_DETECT_PORT               0                /*SMART0 Detect IO Port*/
#define GPIO_SC0_DETECT_BIT                7



/******************************  PIO_1 引脚用法定义*********************************/

#define GPIO_SC1UART_TXD_PORT              1                /*SMART1 Uart Data TXD IO Port*/
#define GPIO_SC1UART_TXD_BIT               0

#define GPIO_SC1UART_RXD_PORT              1                /*SMART1 Uart Data RXD IO Port*/
#define GPIO_SC1UART_RXD_BIT               1

//cs added for NEW Grass Board.20091120
#ifdef GMOON_IBOX_VER1_0
#define GPIO_CI_DETECT_PORT                (1)              /*CI_DET*/
#define GPIO_CI_DETECT_BIT                 (2)
#endif

#ifdef GRASS_IBOXHD9_VER1_1
#define GPIO_TUNER_FIRST_LNB13_18_PORT           1                /*TUNER的LNB 14/19控制所在的IO  口*/
#define GPIO_TUNER_FIRST_LNB13_18_BIT            2
#endif


#define GPIO_SC1_CLK_PORT                  1                /*SMART1 CLK IO Port*/
#define GPIO_SC1_CLK_BIT                   3

#define GPIO_SC1_RST_PORT                  1                /*SMART1 RST IO Port*/
#define GPIO_SC1_RST_BIT                   4

#define GPIO_SC1_VCC_PORT                  1                /*SMART1 VCC IO Port*/
#define GPIO_SC1_VCC_BIT                   5

//cs added for NEW Grass Board.20091120
#ifdef GMOON_IBOX_VER1_0
#define GPIO_CI_PASS_PORT                  (1)              /*Ci pass的IO  口*/
#define GPIO_CI_PASS_BIT                   (6)
#endif

#ifdef GRASS_IBOXHD9_VER1_1
#define GPIO_TUNER_FIRST_LNB_POWER_PORT                1                /*SET LNB_POWER 所在的PIO 口*/
#define GPIO_TUNER_FIRST_LNB_POWER_BIT                 6
#endif


#define GPIO_SC1_DETECT_PORT               1                /*SMART1 Detect IO Port*/
#define GPIO_SC1_DETECT_BIT                7



/******************************  PIO_2 引脚用法定义*********************************/

#define GPIO_I2C_FIRST_SCL_PORT            2                       /*I2C[0] 的CLK IO  口*/
#define GPIO_I2C_FIRST_SCL_BIT             0

#define GPIO_I2C_FIRST_SDA_PORT            2                       /*I2C[0] 的Data IO  口*/
#define GPIO_I2C_FIRST_SDA_BIT             1

/*CS added for AVOutput and other PIO Ctrl. 20091013*/
#ifdef USE_HC595
#define GPIO_HC595_SRCK0_PORT              (2)
#define GPIO_HC595_SRCK0_BIT               (2)

#define GPIO_HC595_RCK0_PORT               (2)
#define GPIO_HC595_RCK0_BIT                (3)

#define GPIO_HC595_DS0_PORT                (2)
#define GPIO_HC595_DS0_BIT                 (4)

#define GPIO_HC595_SRCK1_PORT              (2)
#define GPIO_HC595_SRCK1_BIT               (5)

#define GPIO_HC595_RCK1_PORT               (2)
#define GPIO_HC595_RCK1_BIT                (6)

#define GPIO_HC595_DS1_PORT                (2)
#define GPIO_HC595_DS1_BIT                 (7)

#endif

#ifdef USE_STV6418

#define GPIO_TUNER_SECOND_RESET_PORT       2                       /*CAB复位所在的IO  口*/
#define GPIO_TUNER_SECOND_RESET_BIT        2

#define GPIO_TUNER_FIRST_RESET_PORT        2                       /*TUNER复位所在的IO  口*/
#define GPIO_TUNER_FIRST_RESET_BIT         3

#define GPIO_TUNER_LNB14_19_PORT           2                       /*TUNER的LNB 14/19控制所在的IO  口*/
#define GPIO_TUNER_LNB14_19_BIT            4

#define GPIO_CI_RESET_PORT                 2                       /*CI_RESET*/
#define GPIO_CI_RESET_BIT                  5

#define GPIO_DISEQC_IORX_PORT              2                       /*DISEQC_IORX所在的IO  口*/
#define GPIO_DISEQC_IORX_BIT               6

#define GPIO_HDD_VCC_PORT                  2                       /* HDD VCC Enable.*/
#define GPIO_HDD_VCC_BIT                   7
#endif
/*End add.*/

/******************************  PIO_3 引脚用法定义*********************************/

#define GPIO_I2C_SECOND_SCL_PORT           3                       /*I2C[1] 的CLK IO  口*/
#define GPIO_I2C_SECOND_SCL_BIT            0

#define GPIO_I2C_SECOND_SDA_PORT           3                       /*I2C[1] 的Data IO  口*/
#define GPIO_I2C_SECOND_SDA_BIT            1

#if 0
#define GPIO_UART_DATA_TXD_PORT            3                       /*串口TXD 所在的IO  口*/
#define GPIO_UART_DATA_TXD_BIT             0

#define GPIO_UART_DATA_RXD_PORT            3                       /*串口RXD 所在的IO  口*/
#define GPIO_UART_DATA_RXD_BIT             1
#endif

//cs added for NEW Grass Board.20091120
#ifdef GRASS_IBOXHD9_VER1_1
#define GPIO_CI_PASS_PORT                  3                       /*Ci pass的IO  口*/
#define GPIO_CI_PASS_BIT                   2
#endif


#define GPIO_BLAST_RXD_PORT                3                       /*ir使用的IO口*/
#define GPIO_BLAST_RXD_BIT                 3

#ifdef GRASS_IBOXHD9_VER1_1
#define GPIO_CI_DETECT_PORT                3                       /*CI_DET*/
#define GPIO_CI_DETECT_BIT                 4

#define GPIO_CI_READY_PORT                 3                       /*CI_READY*/
#define GPIO_CI_READY_BIT                  5
#endif


#define GPIO_PHY_RESET_PORT                3                       /*PHY Reset 的 IO  口*/
#define GPIO_PHY_RESET_BIT                 6

/******************************  PIO_4 引脚用法定义*********************************/

#define GPIO_I2C_THIRD_SCL_PORT           4                       /*I2C[2] 的CLK IO  口*/
#define GPIO_I2C_THIRD_SCL_BIT            0

#define GPIO_I2C_THIRD_SDA_PORT           4                       /*I2C[2] 的Data IO  口*/
#define GPIO_I2C_THIRD_SDA_BIT            1


//cs added for NEW Grass Board.20091120
#ifdef GMOON_IBOX_VER1_0
#define GPIO_UART0_DATA_RXD_PORT         (4)                      /*串口0 RXD 所在的IO  口*/
#define GPIO_UART0_DATA_RXD_BIT          (2)

#define GPIO_UART0_DATA_TXD_PORT         (4)                      /*串口0 TXD 所在的IO  口*/
#define GPIO_UART0_DATA_TXD_BIT          (3)

#define GPIO_UART0_CTSD_PORT             (4)                      /*串口0 CTSD 所在的IO  口*/
#define GPIO_UART0_CTSD_BIT              (4)                      /*NOT USED*/

#define GPIO_UART0_RTSD_PORT             (4)                      /*串口0 RTSD 所在的IO  口*/
#define GPIO_UART0_RTSD_BIT              (5)                      /*NOT USED*/
#endif

#if defined(GRASS_IBOXHD9_VER1_1) || defined(MOON_IBOX_VER1_1)
#define GPIO_UART_DATA_RXD_PORT          (4)                       /*串口RXD 所在的IO  口*/
#define GPIO_UART_DATA_RXD_BIT           (2)

#define GPIO_UART_DATA_TXD_PORT          (4)                       /*串口TXD 所在的IO  口*/
#define GPIO_UART_DATA_TXD_BIT           (3)

#define GPIO_TUNER_SET12V_PORT           (4)                       /*12V 供电的IO  口*/
#define GPIO_TUNER_SET12V_BIT            (6)
#endif

//end add

#define GPIO_HDMI_HPD_PORT                4                       /*HDMI 控制*/
#define GPIO_HDMI_HPD_BIT                 7

/******************************  PIO_5 引脚用法定义*********************************/
//cs added for NEW Grass Board.20091120
#ifdef GMOON_IBOX_VER1_0
#define GPIO_UART1_DATA_TXD_PORT         (5)                      /*串口1 TXD 所在的IO  口*/
#define GPIO_UART1_DATA_TXD_BIT          (0)

#define GPIO_UART1_DATA_RXD_PORT         (5)                      /*串口1 RXD 所在的IO  口*/
#define GPIO_UART1_DATA_RXD_BIT          (1)

#define GPIO_UART1_CTSD_PORT             (5)                      /*串口1 CTSD 所在的IO  口*/
#define GPIO_UART1_CTSD_BIT              (2)                      /*NOT USED*/

#define GPIO_UART1_RTSD_PORT             (5)                      /*串口1 RTSD 所在的IO  口*/
#define GPIO_UART1_RTSD_BIT              (3)                      /*NOT USED*/

#define GPIO_HDMI_CEC_PORT               (5)                      /*HDMI CTRL 所在的IO  口*/
#define GPIO_HDMI_CEC_BIT                (5)

#define GPIO_USB_EN_PORT                 (5)                      /*USB 电源使能，1 有效*/
#define GPIO_USB_EN_BIT                  (6)

#define GPIO_USB_PWR_PORT                (5)                      /*USB 过载检测，0 有效*/
#define GPIO_USB_PWR_BIT                 (7)
#endif

/*CS added for AVOutput and other PIO Ctrl. 20091013*/
#ifdef USE_HC595
#define GPIO_VCR_DET_PORT                (5)                      /*VCR DET 所在的IO  口*/
#define GPIO_VCR_DET_BIT                 (4)
#endif
/*end add*/

#ifdef GRASS_IBOXHD9_VER1_1
#define GPIO_TUNER_SWITCH_PORT             5                       /*TUNER SWITCH的IO  口*/
#define GPIO_TUNER_SWITCH_BIT              6
#endif
//end add







/******************************  PIO_6 引脚用法定义*********************************/



/*********************************************************************************************/


/******************************  PIO_ALL INDEX 用法定义*********************************/

//cs added for NEW Grass Board.20091120
#ifdef GMOON_IBOX_VER1_0
#define GPIO_SC0UART_RXD_INDEX              ( GPIO_SC0UART_RXD_PORT*PIO_BITS + GPIO_SC0UART_RXD_BIT )
#define GPIO_SC0UART_TXD_INDEX              ( GPIO_SC0UART_TXD_PORT*PIO_BITS + GPIO_SC0UART_TXD_BIT )
#define GPIO_SC1UART_RXD_INDEX              ( GPIO_SC1UART_RXD_PORT*PIO_BITS + GPIO_SC1UART_RXD_BIT )
#define GPIO_SC1UART_TXD_INDEX              ( GPIO_SC1UART_TXD_PORT*PIO_BITS + GPIO_SC1UART_TXD_BIT )
#define GPIO_PHY_RESET_INDEX                ( GPIO_PHY_RESET_PORT *PIO_BITS + GPIO_PHY_RESET_BIT)
#define GPIO_CA_SW_INDEX                    ( GPIO_CA_SW_PORT*PIO_BITS + GPIO_CA_SW_BIT)
#endif

#ifdef GRASS_IBOXHD9_VER1_1
#define GPIO_TUNER_FIRST_LNB13_18_INDEX           ( GPIO_TUNER_FIRST_LNB13_18_PORT*PIO_BITS + GPIO_TUNER_FIRST_LNB13_18_BIT )
#define GPIO_TUNER_FIRST_LNB_POWER_INDEX          ( GPIO_TUNER_FIRST_LNB_POWER_PORT*PIO_BITS + GPIO_TUNER_FIRST_LNB_POWER_BIT )
#define GPIO_TUNER_SWITCH_INDEX             ( GPIO_TUNER_SWITCH_PORT*PIO_BITS + GPIO_TUNER_SWITCH_BIT )
#define GPIO_VCC_EN3_INDEX                  ( GPIO_VCC_EN3_PORT*PIO_BITS + GPIO_VCC_EN3_BIT)
#define GPIO_CI_DETECT_INDEX                ( GPIO_CI_DETECT_PORT*PIO_BITS + GPIO_CI_DETECT_BIT)
#define GPIO_CI_READY_INDEX                 ( GPIO_CI_READY_PORT*PIO_BITS + GPIO_CI_READY_BIT)
#endif

#define GPIO_CI_PASS_INDEX                  ( GPIO_CI_PASS_PORT*PIO_BITS + GPIO_CI_PASS_BIT )

#if defined(GRASS_IBOXHD9_VER1_1) || defined(MOON_IBOX_VER1_1)
#define GPIO_TUNER_SET_12V_INDEX            ( GPIO_TUNER_SET12V_PORT*PIO_BITS + GPIO_TUNER_SET12V_BIT )
#endif

/*CS added for AVOutput and other PIO Ctrl. 20091013*/

#ifdef USE_STV6418
#define GPIO_TUNER_FIRST_LNB14_19_INDEX           ( GPIO_TUNER_LNB14_19_PORT *PIO_BITS + GPIO_TUNER_LNB14_19_BIT)
#define GPIO_CI_RESET_INDEX                 ( GPIO_CI_RESET_PORT*PIO_BITS + GPIO_CI_RESET_BIT)
#define GPIO_HDD_VCCEN_INDEX                ( GPIO_HDD_VCC_PORT *PIO_BITS + GPIO_HDD_VCC_BIT)
#define GPIO_TUNER_FIRST_RESET_INDEX        ( GPIO_TUNER_FIRST_RESET_PORT*PIO_BITS + GPIO_TUNER_FIRST_RESET_BIT )
#define GPIO_TUNER_SECOND_RESET_INDEX       ( GPIO_TUNER_SECOND_RESET_PORT*PIO_BITS + GPIO_TUNER_SECOND_RESET_BIT )
#define GPIO_DISEQC_IORX_INDEX              ( GPIO_DISEQC_IORX_PORT*PIO_BITS + GPIO_DISEQC_IORX_BIT )
#endif

#ifdef USE_HC595
#define GPIO_HC595_SRCK0_INDEX              ( GPIO_HC595_SRCK0_PORT *PIO_BITS + GPIO_HC595_SRCK0_BIT)
#define GPIO_HC595_RCK0_INDEX               ( GPIO_HC595_RCK0_PORT*PIO_BITS + GPIO_HC595_RCK0_BIT)
#define GPIO_HC595_DS0_INDEX                ( GPIO_HC595_DS0_PORT*PIO_BITS + GPIO_HC595_DS0_BIT)
#define GPIO_HC595_SRCK1_INDEX              ( GPIO_HC595_SRCK1_PORT*PIO_BITS + GPIO_HC595_SRCK1_BIT)
#define GPIO_HC595_RCK1_INDEX               ( GPIO_HC595_RCK1_PORT*PIO_BITS + GPIO_HC595_RCK1_BIT)
#define GPIO_HC595_DS1_INDEX                ( GPIO_HC595_DS1_PORT*PIO_BITS + GPIO_HC595_DS1_BIT)
#define GPIO_VCT_IN_DCT_INDEX               ( GPIO_VCR_DET_PORT*PIO_BITS + GPIO_VCR_DET_BIT)
#endif
/*End add*/
#endif




/**********************************************7111***********************************/
/*************************************************************************************/
/*************************************************************************************/
/*************************************************************************************/
#ifdef CONFIG_CPU_SUBTYPE_STX7111
#define YW_HW_SAT7111_220
#ifdef YW_HW_SAT7111_220
/******************************  PIO_0 引脚用法定义*********************************/
#define GPIO_SC0UART_TXD_PORT              0                /*SMART0 Uart Data TXD IO Port*/
#define GPIO_SC0UART_TXD_BIT               0

#define GPIO_SC0UART_RXD_PORT              0                /*SMART0 Uart Data RXD IO Port*/
#define GPIO_SC0UART_RXD_BIT               1

#define GPIO_SC0_CLK_PORT                  0                /*SMART0 CLK IO Port*/
#define GPIO_SC0_CLK_BIT                   3

#define GPIO_SC0_RST_PORT                  0                /*SMART0 CARD RESET*/
#define GPIO_SC0_RST_BIT                   4                /*用于板载CA 复位 ， 0 有效*/

#define GPIO_SC0_VCC_PORT                  0                /*SMART0 VCC IO Port*/
#define GPIO_SC0_VCC_BIT                   5

#define GPIO_SC0_DETECT_PORT               0                /*SMART0 Detect IO Port*/
#define GPIO_SC0_DETECT_BIT                7

/******************************  PIO_1 引脚用法定义*********************************/
#define GPIO_SC1UART_TXD_PORT              1                /*SMART1 Uart Data TXD IO Port*/
#define GPIO_SC1UART_TXD_BIT               0

#define GPIO_SC1UART_RXD_PORT              1                /*SMART1 Uart Data RXD IO Port*/
#define GPIO_SC1UART_RXD_BIT               1

#define GPIO_SC1_CLK_PORT                  1                /*SMART1 CLK IO Port*/
#define GPIO_SC1_CLK_BIT                   3

#define GPIO_SC1_RST_PORT                  1                /*SMART1 CARD RESET*/
#define GPIO_SC1_RST_BIT                   4                /*用于板载CA 复位 ， 0 有效*/

#define GPIO_SC1_VCC_PORT                  1                /*SMART1 VCC IO Port*/
#define GPIO_SC1_VCC_BIT                   5

#define GPIO_SC1_DETECT_PORT               1                /*SMART1 Detect IO Port，复用脚，跟hdmi cec脚复用*/
#define GPIO_SC1_DETECT_BIT                7




#define GPIO_TUNER_SECOND_LNB14_19_PORT   (1)               /*PIO1.6-- 用于 LNB 14/19控制(Tuner1)*/
#define GPIO_TUNER_SECOND_LNB14_19_BIT    (6)

#define GPIO_HDMI_CEC_PORT                (1)               /*PIO1.7 HDMI CTRL所在的IO口*/
#define GPIO_HDMI_CEC_BIT                 (7)



/******************************  PIO_2 引脚用法定义*********************************/
#define GPIO_TUNER_SET12V_PORT           (2)                       /*12V 供电的IO 口*/
#define GPIO_TUNER_SET12V_BIT            (3)

#define GPIO_AUDIO_MUTE_PORT	         (2)                       /*PIO2.4 ---静音控制*/
#define GPIO_AUDIO_MUTE_BIT	             (4)

#define GPIO_ESAM_RESET_PORT	         (2)                       /*PIO2.5 ---ESAM加密IC复位脚*/
#define GPIO_ESAM_RESET_BIT	             (5)

#define GPIO_CA_ESAM_SW_PORT	         (2)                       /*PIO2.6 ---CA与ESAM加密IC的切换脚*/
#define GPIO_CA_ESAM_SW_BIT	             (6)

#if 1
#define GPIO_TUNER_SECOND_RESET_PORT     (2)                      /*天线2的复位控制*/
#define GPIO_TUNER_SECOND_RESET_BIT      (7)
#endif

/******************************  PIO_3 引脚用法定义*********************************/

#define GPIO_I2C_FIRST_SCL_PORT           3                       /*I2C[0] 的CLK IO  口*/
#define GPIO_I2C_FIRST_SCL_BIT            0

#define GPIO_I2C_FIRST_SDA_PORT           3                       /*I2C[0] 的Data IO  口*/
#define GPIO_I2C_FIRST_SDA_BIT            1

#define GPIO_BLAST_RXD_PORT               3                       /*ir使用的IO口*/
#define GPIO_BLAST_RXD_BIT                3



/******************************  PIO_4 引脚用法定义*********************************/
#define GPIO_I2C_SECOND_SCL_PORT         (4)                      /*I2C[1] 的CLK IO  口*/
#define GPIO_I2C_SECOND_SCL_BIT          (0)

#define GPIO_I2C_SECOND_SDA_PORT         (4)                      /*I2C[1] 的Data IO  口*/
#define GPIO_I2C_SECOND_SDA_BIT          (1)

#define GPIO_UART0_DATA_RXD_PORT         (4)                      /*232串口 RXD 所在的IO  口*/
#define GPIO_UART0_DATA_RXD_BIT          (2)

#define GPIO_UART0_DATA_TXD_PORT         (4)                      /*232串口 TXD 所在的IO  口*/
#define GPIO_UART0_DATA_TXD_BIT          (3)

#define GPIO_UART0_CTSD_PORT             (4)                      /*232 串口 CTSD 所在的IO  口*/
#define GPIO_UART0_CTSD_BIT              (4)

#define GPIO_UART0_RTSD_PORT             (4)                      /*232 串口 RTSD 所在的IO  口*/
#define GPIO_UART0_RTSD_BIT              (5)

#define GPIO_TUNER_SECOND_LNB13_18_PORT  (4)                      /*PIO4.6-- 用于 LNB 13/18控制(Tuner1)*/
#define GPIO_TUNER_SECOND_LNB13_18_BIT   (6)

#define GPIO_HDMI_HPD_PORT               (4)                      /*HDMI热插拔检测*/
#define GPIO_HDMI_HPD_BIT                (7)


/******************************  PIO_5 引脚用法定义*********************************/
#define GPIO_PHY_RESET_PORT              (5)                      /*PHY Reset 的 IO  口*/
#define GPIO_PHY_RESET_BIT               (0)

#define GPIO_TUNER_SECOND_LNB_POWER_PORT (5)                      /*卫星天线2的LNB开关脚*/
#define GPIO_TUNER_SECOND_LNB_POWER_BIT  (2)

#if 0
#define GPIO_TUNER_SECOND_RESET_PORT     (5)                      /*天线2的复位控制*/
#define GPIO_TUNER_SECOND_RESET_BIT      (3)
#endif
#define GPIO_HDMI_EN_PORT                (5)                      /*HDMI使能控制*/
#define GPIO_HDMI_EN_BIT                 (4)

#if 0
#define GPIO_HDMI_PWR_PORT               (5)                      /*HDMI过流保护控制*/
#define GPIO_HDMI_PWR_BIT                (5)
#else
#define GPIO_TUNER_FIRST_LNB14_19_PORT   (5)                      /*PIO5.5-- 用于 LNB 14/19控制(Tuner0)*/
#define GPIO_TUNER_FIRST_LNB14_19_BIT    (5)
#endif

#define GPIO_USB_PWR_PORT                (5)                      /*USB 过载检测，0 有效*/
#define GPIO_USB_PWR_BIT                 (6)

#define GPIO_USB_EN_PORT                 (5)                      /*USB 电源使能，1 有效*/
#define GPIO_USB_EN_BIT                  (7)


/******************************  PIO_6 引脚用法定义*********************************/

#define  GPIO_CVBS_RGB_SW_PORT	        (6)				/* 用于CVBS/RGB切换*/
#define  GPIO_CVBS_RGB_SW_BIT	        (0)

#define  GPIO_ON_STBY_PORT	            (6)				/* TV/STA*/
#define  GPIO_ON_STBY_BIT	            (1)

#define  GPIO_SCART_169_43_PORT	        (6)				/* 16:9控制脚*/
#define  GPIO_SCART_169_43_BIT	        (2)

#if 1
#define GPIO_TUNER_FIRST_LNB_POWER_PORT (6)                 /*卫星天线1的LNB开关脚*/
#define GPIO_TUNER_FIRST_LNB_POWER_BIT  (5)

#define GPIO_TUNER_FIRST_LNB13_18_PORT  (6)                 /*卫星天线1的极化方向控制*/
#define GPIO_TUNER_FIRST_LNB13_18_BIT   (6)
#else //before version my board
#define GPIO_TUNER_FIRST_LNB_POWER_PORT (1)                 /*卫星天线1的LNB开关脚*/
#define GPIO_TUNER_FIRST_LNB_POWER_BIT  (2)

#define GPIO_TUNER_FIRST_LNB13_18_PORT  (0)                 /*卫星天线1的极化方向控制*/
#define GPIO_TUNER_FIRST_LNB13_18_BIT   (2)

#endif
#if 0
#define GPIO_SCART_VCR_DECT_PORT		   6
#define GPIO_SCART_VCR_DECT_BIT			   3
#endif
#define GPIO_TV_VCR_SWITCH_PORT			   6
#define GPIO_TV_VCR_SWITCH_BIT			   4
/*********************************************************************************************/
#define GPIO_I2C_THIRD_SCL_BIT            0
#define GPIO_I2C_THIRD_SDA_BIT            1

/******************************  PIO_ALL INDEX 用法定义*********************************/

#define GPIO_AUDIO_MUTE_INDEX                (GPIO_AUDIO_MUTE_PORT*PIO_BITS + GPIO_AUDIO_MUTE_BIT)

#define GPIO_TUNER_FIRST_LNB13_18_INDEX      (GPIO_TUNER_FIRST_LNB13_18_PORT*PIO_BITS + GPIO_TUNER_FIRST_LNB13_18_BIT )
#define GPIO_TUNER_SECOND_LNB13_18_INDEX     (GPIO_TUNER_SECOND_LNB13_18_PORT*PIO_BITS + GPIO_TUNER_SECOND_LNB13_18_BIT )

#define GPIO_TUNER_FIRST_LNB14_19_INDEX    ( GPIO_TUNER_FIRST_LNB14_19_PORT *PIO_BITS + GPIO_TUNER_FIRST_LNB14_19_BIT)
#define GPIO_TUNER_SECOND_LNB14_19_INDEX     ( GPIO_TUNER_SECOND_LNB14_19_PORT *PIO_BITS + GPIO_TUNER_SECOND_LNB14_19_BIT)

#define GPIO_TUNER_FIRST_LNB_POWER_INDEX     ( GPIO_TUNER_FIRST_LNB_POWER_PORT*PIO_BITS + GPIO_TUNER_FIRST_LNB_POWER_BIT )
#define GPIO_TUNER_SECOND_LNB_POWER_INDEX    ( GPIO_TUNER_SECOND_LNB_POWER_PORT*PIO_BITS + GPIO_TUNER_SECOND_LNB_POWER_BIT )

//#define GPIO_TUNER_FIRST_RESET_INDEX       ( GPIO_TUNER_FIRST_RESET_PORT*PIO_BITS + GPIO_TUNER_FIRST_RESET_BIT )
#define GPIO_TUNER_SECOND_RESET_INDEX        ( GPIO_TUNER_SECOND_RESET_PORT*PIO_BITS + GPIO_TUNER_SECOND_RESET_BIT )

#define GPIO_TUNER_SET_12V_INDEX             ( GPIO_TUNER_SET12V_PORT*PIO_BITS + GPIO_TUNER_SET12V_BIT )

#define GPIO_PHY_RESET_INDEX                 ( GPIO_PHY_RESET_PORT *PIO_BITS + GPIO_PHY_RESET_BIT)

//#define GPIO_DISEQC_IOTX_INDEX             ( GPIO_DISEQC_IOTX_PORT*PIO_BITS + GPIO_DISEQC_IOTX_BIT )

//#define GPIO_DISEQC_IORX_INDEX             ( GPIO_DISEQC_IORX_PORT*PIO_BITS + GPIO_DISEQC_IORX_BIT )





//#define GPIO_CI_PASS_INDEX                  ( GPIO_CI_PASS_PORT*PIO_BITS + GPIO_CI_PASS_BIT )

//#define GPIO_TUNER_SWITCH_INDEX            ( GPIO_TUNER_SWITCH_PORT*PIO_BITS + GPIO_TUNER_SWITCH_BIT )

//#define GPIO_SC0UART_RXD_INDEX              ( GPIO_SC0UART_RXD_PORT*PIO_BITS + GPIO_SC0UART_RXD_BIT )

//#define GPIO_SC0UART_TXD_INDEX              ( GPIO_SC0UART_TXD_PORT*PIO_BITS + GPIO_SC0UART_TXD_BIT )

//#define GPIO_SC1UART_RXD_INDEX              ( GPIO_SC1UART_RXD_PORT*PIO_BITS + GPIO_SC1UART_RXD_BIT )

//#define GPIO_SC1UART_TXD_INDEX              ( GPIO_SC1UART_TXD_PORT*PIO_BITS + GPIO_SC1UART_TXD_BIT )

//#define GPIO_BLAST_RXD_INDEX               ( GPIO_BLAST_RXD_PORT*PIO_BITS + GPIO_BLAST_RXD_BIT )

//#define GPIO_BLAST_TXD_INDEX               ( GPIO_BLAST_TXD_PORT*PIO_BITS + GPIO_BLAST_RXD_BIT )
//#define GPIO_HDD_VCCEN_INDEX                ( GPIO_HDD_VCC_PORT *PIO_BITS + GPIO_HDD_VCC_BIT)

//d48zm 2009.12.9 add
#define GPIO_CVBS_RGB_SWITCH_INDEX		   	(GPIO_CVBS_RGB_SW_PORT*PIO_BITS + GPIO_CVBS_RGB_SW_BIT)
#define GPIO_ON_STBY_INDEX                	(GPIO_ON_STBY_PORT*PIO_BITS + GPIO_ON_STBY_BIT)
#define GPIO_169_43_INDEX				   	(GPIO_SCART_169_43_PORT*PIO_BITS + GPIO_SCART_169_43_BIT)
#if 0
#define GPIO_SCART_VCR_DECT_INDEX		    (GPIO_SCART_VCR_DECT_PORT*PIO_BITS + GPIO_SCART_VCR_DECT_BIT)
#define GPIO_TV_VCR_SWITCH_INDEX		    (GPIO_TV_VCR_SWITCH_PORT*PIO_BITS + GPIO_TV_VCR_SWITCH_BIT)
#endif
//d48zm add end

#endif
#endif

#ifdef CONFIG_CPU_SUBTYPE_STX7105


/******************************  PIO_0 引脚用法定义*********************************/
#define GPIO_SC0UART_TXD_PORT              0                /*SMART0 Uart Data TXD IO Port*/
#define GPIO_SC0UART_TXD_BIT               0

#define GPIO_SC0UART_RXD_PORT              0                /*SMART0 Uart Data RXD IO Port*/
#define GPIO_SC0UART_RXD_BIT               1

#define GPIO_SC0_CLK_PORT                  0                /*SMART0 CLK IO Port*/
#define GPIO_SC0_CLK_BIT                   3

#define GPIO_SC0_RST_PORT                  0                /*SMART0 RST IO Port*/
#define GPIO_SC0_RST_BIT                   4

#define GPIO_SC0_VCC_PORT                  0                /*SMART0 VCC IO Port*/
#define GPIO_SC0_VCC_BIT                   5

#define GPIO_SC0_DETECT_PORT               0                /*SMART0 Detect IO Port*/
#define GPIO_SC0_DETECT_BIT                7

/******************************  PIO_1 引脚用法定义*********************************/

#define GPIO_SC1UART_TXD_PORT              1                /*SMART1 Uart Data TXD IO Port*/
#define GPIO_SC1UART_TXD_BIT               0

#define GPIO_SC1UART_RXD_PORT              1                /*SMART1 Uart Data RXD IO Port*/
#define GPIO_SC1UART_RXD_BIT               1

//#define GPIO_SC1_CLKGenExtClk_PORT         1                /*SMART1 CLKGenExtCLK Port*/
//#define GPIO_SC1_CLKGenExtClk_BIT          2

#define GPIO_SC1_CLK_PORT                  1                /*SMART1 CLK IO Port*/
#define GPIO_SC1_CLK_BIT                   3

#define GPIO_SC1_RST_PORT                  1                /*SMART1 RST IO Port*/
#define GPIO_SC1_RST_BIT                   4

#define GPIO_SC1_VCC_PORT                  1                /*SMART1 VCC IO Port*/
#define GPIO_SC1_VCC_BIT                   5

#define GPIO_SC1_DETECT_PORT               1                /*SMART1 Detect IO Port*/
#define GPIO_SC1_DETECT_BIT                7

/******************************  PIO_2 引脚用法定义*********************************/
#define GPIO_SIMULATE_I2C_SCL_PORT           2                    /*模拟I2C的GPIO口 SCL*/
#define GPIO_SIMULATE_I2C_SCL_BIT            0

#define GPIO_SIMULATE_I2C_SDA_PORT           2                    /*模拟I2C的GPIO口 SDA*/
#define GPIO_SIMULATE_I2C_SDA_BIT           1

#define GPIO_I2C_FIRST_SCL_PORT           2                       /*I2C[0] 的CLK IO  口,used for hdmi auto detect*/
#define GPIO_I2C_FIRST_SCL_BIT            2

#define GPIO_I2C_FIRST_SDA_PORT           2                       /*I2C[0] 的Data IO  口*/
#define GPIO_I2C_FIRST_SDA_BIT            3

#define GPIO_I2C_SECOND_SCL_PORT           2                       /*I2C[1] 的CLK IO  口,used for S:tuner0*/
#define GPIO_I2C_SECOND_SCL_BIT            5

#define GPIO_I2C_SECOND_SDA_PORT           2                       /*I2C[1] 的Data IO  口*/
#define GPIO_I2C_SECOND_SDA_BIT            6

/******************************  PIO_3 引脚用法定义*********************************/
#define GPIO_BLAST_RXD_PORT               3                       /*ir使用的IO口 RX pin*/
#define GPIO_BLAST_RXD_BIT                0

//#define GPIO_BLAST_TXD_PORT               3                       /*ir使用的IO口 TX pin*/
//#define GPIO_BLAST_TXD_BIT                2

#define GPIO_TUNER_SECOND_RESET_PORT       3                     /*TUNER1复位所在的IO T/C:i2c2)*/
#define GPIO_TUNER_SECOND_RESET_BIT        3


#define GPIO_I2C_THIRD_SCL_PORT         (3)                      /*I2C[2] 的CLK IO  口,used for tuner T/C*/
#define GPIO_I2C_THIRD_SCL_BIT          (4)

#define GPIO_I2C_THIRD_SDA_PORT         (3)                      /*I2C[2] 的Data IO  口*/
#define GPIO_I2C_THIRD_SDA_BIT          (5)


#define GPIO_I2C_FOURTH_SCL_PORT        (3)                      /*I2C[3] 的CLK IO  口 used for S:tuner1*/
#define GPIO_I2C_FOURTH_SCL_BIT         (6)

#define GPIO_I2C_FOURTH_SDA_PORT        (3)                      /*I2C[3] 的Data IO  口*/
#define GPIO_I2C_FOURTH_SDA_BIT         (7)


/******************************  PIO_4 引脚用法定义*********************************/
#define GPIO_UART0_DATA_RXD_PORT         (4)                      /*232串口 RXD 所在的IO  口*/
#define GPIO_UART0_DATA_RXD_BIT          (0)

#define GPIO_UART0_DATA_TXD_PORT         (4)                      /*232串口 TXD 所在的IO  口*/
#define GPIO_UART0_DATA_TXD_BIT          (1)

#define GPIO_USB0_OC_PORT                (4)                      /*USB0 过载检测，0 有效*/
#define GPIO_USB0_OC_BIT                 (4)

#define GPIO_USB0_PWR_PORT                 (4)                      /*USB0 电源使能，1 有效*/
#define GPIO_USB0_PWR_BIT                  (5)

#define GPIO_USB1_OC_PORT                (4)                      /*USB1 过载检测，0 有效*/
#define GPIO_USB1_OC_BIT                 (6)

#define GPIO_USB1_PWR_PORT                 (4)                      /*USB1 电源使能，1 有效*/
#define GPIO_USB1_PWR_BIT                  (7)


/******************************  PIO_5 引脚用法定义*********************************/
#define GPIO_TUNER_FIRST_RESET_PORT        5                     /*TUNER0复位所在的IO  口:S:TUNER0(I2C1)*/
#define GPIO_TUNER_FIRST_RESET_BIT         5

#define GPIO_TUNER_THIRD_RESET_PORT        5                     /*TUNER2复位所在的IO  口:S:TUNER1(I2C3)*/
#define GPIO_TUNER_THIRD_RESET_BIT         6

#define GPIO_PHY_RESET_PORT                5                       /*PHY Reset 的 IO  口*/
#define GPIO_PHY_RESET_BIT                 7


/******************************  PIO_9 引脚用法定义*********************************/
#define GPIO_HDMI_HPD_PORT                9                      /*HDMI 控制*/
#define GPIO_HDMI_HPD_BIT                 7


/******************************* PIO11  引脚定义 ************************************/
#define GPIO_AUDIO_MUTE_PORT	         (11)                       /*PIO11.2 ---静音控制*/
#define GPIO_AUDIO_MUTE_BIT	             (2)

#define  GPIO_ON_STBY_PORT	            (11)				/* TV/STA*/
#define  GPIO_ON_STBY_BIT	            (3)

#define  GPIO_SCART_169_43_PORT	        (11)				/* 16:9控制脚*/
#define  GPIO_SCART_169_43_BIT	        (4)

#define  GPIO_CVBS_RGB_SW_PORT	        (11)				/* 用于CVBS/RGB切换*/
#define  GPIO_CVBS_RGB_SW_BIT	        (5)

#if 0
#define GPIO_TUNER_SET12V_PORT           (11)               /*12V 供电的 IO  口*/ //一用PIO11.7就有问题，Ｔ的tuner就i2c出错。
#define GPIO_TUNER_SET12V_BIT            (6) //lwj change 7 to 6 for demo board
#else
#define GPIO_TUNER_SET12V_PORT           (11)               /*12V 供电的 IO  口*/ //一用PIO11.7就有问题，Ｔ的tuner就i2c出错。
#define GPIO_TUNER_SET12V_BIT            (7)

#endif

/******************************* PIO15  引脚定义 ************************************/
#if 0 //for demo board
#define GPIO_TUNER_SECOND_LNB14_19_PORT   (15)                      /*PIO15.4-- 用于 LNB 14/19控制(S:Tuner1)*/ //PIO15.4内核用过了，所以会有问题。建议改成其它的
#define GPIO_TUNER_SECOND_LNB14_19_BIT    (2) //lwj for demo board

#define GPIO_TUNER_SECOND_LNB_POWER_PORT   (15)                     /*PIO15.5-- 用于 LNB开关脚(S:Tuner1)*/
#define GPIO_TUNER_SECOND_LNB_POWER_BIT     (3)//lwj for demo board

#define GPIO_TUNER_FIRST_LNB14_19_PORT   (15)                      /*PIO15.6-- 用于 LNB 14/19控制(S:Tuner0)*/
#define GPIO_TUNER_FIRST_LNB14_19_BIT    (6)

#define GPIO_TUNER_FIRST_LNB_POWER_PORT   (15)                     /*PIO15.7-- 用于 LNB开关脚(S:Tuner0)*/
#define GPIO_TUNER_FIRST_LNB_POWER_BIT     (7)
#else
#define GPIO_TUNER_SECOND_LNB14_19_PORT   (15)                      /*PIO15.4-- 用于 LNB 14/19控制(S:Tuner1)*/ //PIO15.4内核用过了，所以会有问题。建议改成其它的
#define GPIO_TUNER_SECOND_LNB14_19_BIT    (4)

#define GPIO_TUNER_SECOND_LNB_POWER_PORT   (15)                     /*PIO15.5-- 用于 LNB开关脚(S:Tuner1)*/
#define GPIO_TUNER_SECOND_LNB_POWER_BIT     (5)

#define GPIO_TUNER_FIRST_LNB14_19_PORT   (15)                      /*PIO15.6-- 用于 LNB 14/19控制(S:Tuner0)*/
#define GPIO_TUNER_FIRST_LNB14_19_BIT    (6)

#define GPIO_TUNER_FIRST_LNB_POWER_PORT   (15)                     /*PIO15.7-- 用于 LNB开关脚(S:Tuner0)*/
#define GPIO_TUNER_FIRST_LNB_POWER_BIT     (7)

#endif
/******************************* PIO15  引脚定义 end************************************/



#define GPIO_TUNER_FIRST_RESET_INDEX       ( GPIO_TUNER_FIRST_RESET_PORT*PIO_BITS + GPIO_TUNER_FIRST_RESET_BIT )
#define GPIO_TUNER_SECOND_RESET_INDEX      ( GPIO_TUNER_SECOND_RESET_PORT*PIO_BITS + GPIO_TUNER_SECOND_RESET_BIT )
#define GPIO_TUNER_THIRD_RESET_INDEX       ( GPIO_TUNER_THIRD_RESET_PORT*PIO_BITS + GPIO_TUNER_THIRD_RESET_BIT )


#define GPIO_TUNER_FIRST_LNB14_19_INDEX    ( GPIO_TUNER_FIRST_LNB14_19_PORT *PIO_BITS + GPIO_TUNER_FIRST_LNB14_19_BIT)
#define GPIO_TUNER_SECOND_LNB14_19_INDEX     ( GPIO_TUNER_SECOND_LNB14_19_PORT *PIO_BITS + GPIO_TUNER_SECOND_LNB14_19_BIT)

#define GPIO_TUNER_FIRST_LNB_POWER_INDEX     ( GPIO_TUNER_FIRST_LNB_POWER_PORT*PIO_BITS + GPIO_TUNER_FIRST_LNB_POWER_BIT )
#define GPIO_TUNER_SECOND_LNB_POWER_INDEX    ( GPIO_TUNER_SECOND_LNB_POWER_PORT*PIO_BITS + GPIO_TUNER_SECOND_LNB_POWER_BIT )

#define GPIO_TUNER_SET_12V_INDEX             ( GPIO_TUNER_SET12V_PORT*PIO_BITS + GPIO_TUNER_SET12V_BIT )

#define GPIO_PHY_RESET_INDEX                 ( GPIO_PHY_RESET_PORT *PIO_BITS + GPIO_PHY_RESET_BIT)

#define GPIO_AUDIO_MUTE_INDEX                (GPIO_AUDIO_MUTE_PORT*PIO_BITS + GPIO_AUDIO_MUTE_BIT)

#define GPIO_CVBS_RGB_SWITCH_INDEX		   	(GPIO_CVBS_RGB_SW_PORT*PIO_BITS + GPIO_CVBS_RGB_SW_BIT)
#define GPIO_ON_STBY_INDEX                	(GPIO_ON_STBY_PORT*PIO_BITS + GPIO_ON_STBY_BIT)
#define GPIO_169_43_INDEX				   	(GPIO_SCART_169_43_PORT*PIO_BITS + GPIO_SCART_169_43_BIT)

#endif



S8*  YWGPIOi_GetPortDeviceName(U32 PortIndex);

#ifdef __cplusplus
}
#endif

#endif


