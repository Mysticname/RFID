#include <reg52.h>
#include <intrins.h>

//#region W5500寄存器地址定义
// W5500寄存器地址定义
#define W5500_MR        0x0000  // 模式寄存器
#define W5500_GAR       0x0001  // 网关地址寄存器
#define W5500_SUBR      0x0005  // 子网掩码寄存器
#define W5500_SHAR      0x0009  // 源MAC地址寄存器
#define W5500_SIPR      0x000F  // 源IP地址寄存器
#define W5500_RTR       0x0019  // 重传超时寄存器
#define W5500_RCR       0x001B  // 重传计数寄存器

// Socket寄存器偏移
#define Sn_MR           0x0000  // Socket模式寄存器
#define Sn_CR           0x0001  // Socket命令寄存器
#define Sn_IR           0x0002  // Socket中断寄存器
#define Sn_SR           0x0003  // Socket状态寄存器
#define Sn_PORT         0x0004  // Socket端口寄存器
#define Sn_DHAR         0x0006  // 目标MAC地址寄存器
#define Sn_DIPR         0x000C  // 目标IP地址寄存器
#define Sn_DPORT        0x0010  // 目标端口寄存器
#define Sn_MSSR         0x0012  // Socket最大段大小寄存器
#define Sn_TOS          0x0015  // Socket服务类型寄存器
#define Sn_TTL          0x0016  // Socket生存时间寄存器
#define Sn_RXBUF_SIZE   0x001E  // Socket接收缓冲区大小寄存器
#define Sn_TXBUF_SIZE   0x001F  // Socket发送缓冲区大小寄存器
#define Sn_TX_FSR       0x0020  // Socket发送空闲大小寄存器
#define Sn_TX_RD        0x0022  // Socket发送读指针寄存器
#define Sn_TX_WR        0x0024  // Socket发送写指针寄存器
#define Sn_RX_RSR       0x0026  // Socket接收大小寄存器
#define Sn_RX_RD        0x0028  // Socket接收读指针寄存器
#define Sn_RX_WR        0x002A  // Socket接收写指针寄存器
//#endregion

//#region W5500 Socket命令和状态定义
// Socket命令
#define SOCK_OPEN_w55       0x01
#define SOCK_LISTEN_w55     0x02
#define SOCK_CONNECT_w55    0x04
#define SOCK_DISCON_w55     0x08
#define SOCK_CLOSE_w55      0x10
#define SOCK_SEND_w55       0x20
#define SOCK_SEND_MAC_w55   0x21
#define SOCK_SEND_KEEP_w55  0x22
#define SOCK_RECV_w55       0x40

// Socket状态
#define SOCK_CLOSED_w55     0x00
#define SOCK_INIT_w55       0x13
#define SOCK_LISTENING_w55  0x14  // 修复：避免与SOCK_LISTEN命令冲突
#define SOCK_SYNSENT_w55    0x15
#define SOCK_SYNRECV_w55    0x16
#define SOCK_ESTABLISHED_w55 0x17
#define SOCK_FIN_WAIT_w55   0x18
#define SOCK_CLOSING_w55    0x1A
#define SOCK_TIME_WAIT_w55  0x1B
#define SOCK_CLOSE_WAIT_w55 0x1C
#define SOCK_LAST_ACK_w55   0x1D
#define SOCK_UDP_w55        0x22
#define SOCK_IPRAW_w55      0x32
#define SOCK_MACRAW_w55     0x42
#define SOCK_PPPOE_w55      0x5F
//#endregion

//#region RC522引脚定义
// RC522引脚定义
sbit RC522_SDA  = P1^0;
sbit RC522_SCK  = P1^1;
sbit RC522_MOSI = P1^2;
sbit RC522_MISO = P1^3;
sbit RC522_RST  = P1^4;
//#endregion

//#region W5500引脚定义
// W5500引脚定义
sbit W5500_SCLK = P2^0;  // SPI时钟
sbit W5500_SCS  = P2^1;  // SPI片选
sbit W5500_MOSI = P2^2;  // 主出从入
sbit W5500_MISO = P2^3;  // 主入从出
sbit W5500_INT  = P2^4;  // 中断引脚 - 未使用
sbit W5500_RST  = P2^5;  // 复位引脚
//#endregion

//#region W5500网络配置变量
// 网络配置 - 根据您的网络环境配置
// 设置W5500的IP地址，与您的电脑在同一网段
unsigned char local_ip_w55[4] = {192, 168, 31, 56};     // W5500的IP地址
unsigned char subnet_w55[4] = {255, 255, 255, 0};        // 子网掩码，与您的网络一致
unsigned char gateway_w55[4] = {192, 168, 31, 1};      // 网关地址，与您的默认网关一致
unsigned char mac_addr_w55[6] = {0x00, 0x08, 0xDC, 0x11, 0x11, 0x11}; // MAC地址

//#region 控制指令定义
// 控制指令定义
#define CMD_ERROR    '0'
#define CMD_CONTROL1 '1'
#define CMD_CONTROL2 '2'
#define CMD_CONTROL3 '3'

// 控制状态结构体
typedef struct {
    unsigned char device_status;      // 设备状态
    unsigned char control1_active;    // 控制1状态
    unsigned char control2_active;    // 控制2状态
    unsigned char control3_active;    // 控制3状态
    unsigned int last_command_time;   // 最后指令时间(简化为16位)
} ControlStatus;

//==============================================================================
// 全局变量
//==============================================================================
ControlStatus control_status = {0, 0, 0, 0, 0};

// 串口初始化标志位
bit uart_initialized = 0;

//==============================================================================
// 函数声明
//==============================================================================
void UART_Send_String_w55(unsigned char *str);
void UART_Send_Char_w55(unsigned char c);
unsigned char receive_control_command_w55(void);
unsigned char process_control_command_w55(unsigned char command);
unsigned char execute_reset_w55(void);
unsigned char Get_Socket_Status_w55(unsigned char socket);
void TCP_Send_Data_w55(unsigned char socket, unsigned char *buf, unsigned int len);
void send_result_to_backend_w55(unsigned char command, unsigned char result);
unsigned int TCP_Recv_Data_w55(unsigned char socket, unsigned char *buf, unsigned int max_len);
//==============================================================================









// ===================================

/**
 * @brief 向后端发送指令执行结果
 * @param command: 原始指令
 * @param result: 执行结果 (1-成功, 0-失败)
 */
void send_result_to_backend_w55(unsigned char command, unsigned char result)
{
    unsigned char response[16];  // 响应缓冲区
    unsigned char response_len = 0;  // 响应长度

    if (result) {
        // ?? 成功响应："OK:X"
        response[response_len++] = 'O';
        response[response_len++] = 'K';
        response[response_len++] = ':';
        response[response_len++] = command;
    } else {
        // ?? 失败响应："ERR:X"
        response[response_len++] = 'E';
        response[response_len++] = 'R';
        response[response_len++] = 'R';
        response[response_len++] = ':';
        response[response_len++] = command;
    }

    response[response_len++] = '\r';
    response[response_len++] = '\n';

    // ?? 通过W5500发送响应到后端
    if (Get_Socket_Status_w55(0) == SOCK_ESTABLISHED_w55) {
        TCP_Send_Data_w55(0, response, response_len);
        UART_Send_String_w55("Result sent to backend: ");
        response[response_len] = '\0';
        UART_Send_String_w55(response);
    }
}

/**
 * @brief 执行复位指令
 */
unsigned char execute_reset_w55(void)
{
    // ?? 复位设备状态
    P3 = 0x00;  // 清除所有输出

    UART_Send_String_w55("Device reset completed\r\n");
    return 1;
}

/**
 * @brief 执行控制指令1
 */
unsigned char execute_control1_w55(void)
{
    // ?? 根据您的实际需求修改
    // P3_0 = 1;  // 控制某个设备

    UART_Send_String_w55("Control 1 executed\r\n");
    return 1;
}

/**
 * @brief 执行控制指令2
 */
unsigned char execute_control2_w55(void)
{
    // ?? 根据您的实际需求修改
    // P3_1 = 1;  // 控制另一个设备

    UART_Send_String_w55("Control 2 executed\r\n");
    return 1;
}

/**
 * @brief 执行控制指令3
 */
unsigned char execute_control3_w55(void)
{
    // ?? 状态查询或其他功能
    unsigned char status = P1;  // 读取状态

    UART_Send_String_w55("Control 3 executed - Status: 0x");
    UART_Send_Char_w55((status >> 4) < 10 ? (status >> 4) + '0' : (status >> 4) - 10 + 'A');
    UART_Send_Char_w55((status & 0x0F) < 10 ? (status & 0x0F) + '0' : (status & 0x0F) - 10 + 'A');
    UART_Send_String_w55("\r\n");

    return 1;
}

/**
 * @brief 处理接收到的控制指令
 * @param command: 控制指令字符 ('0', '1', '2', '3')
 * @return 处理结果: 1-成功, 0-失败
 */
unsigned char process_control_command_w55(unsigned char command)
{
    unsigned char result = 0;

    switch (command) {
        case '0':    // 错误处理/复位
            UART_Send_String_w55("This is dpj received 00000000000000000\r\n");    
            UART_Send_String_w55("This is dpj received 00000000000000000\r\n");            
            UART_Send_String_w55("This is dpj received 00000000000000000\r\n");                        
            result = 1;
            // UART_Send_String_w55("Executing reset command...\r\n");
            // result = execute_reset_w55();
            break;

        case '1':    // 控制指令1
            UART_Send_String_w55("This is dpj received 1111111\r\n");    
            UART_Send_String_w55("This is dpj received 1111111\r\n");
            UART_Send_String_w55("This is dpj received 1111111\r\n");    
            result = 1;        
            // UART_Send_String_w55("Executing control command 1...\r\n");
            // result = execute_control1_w55();
            break;

        case '2':    // 控制指令2
            UART_Send_String_w55("This is dpj received 22222\r\n");
            UART_Send_String_w55("This is dpj received 22222\r\n");
            UART_Send_String_w55("This is dpj received 22222\r\n");
            result = 1;
            // UART_Send_String_w55("Executing control command 2...\r\n");
            // result = execute_control2_w55();
            break;

        case '3':    // 控制指令3
            UART_Send_String_w55("This is dpj received 333333\r\n");
            UART_Send_String_w55("This is dpj received 333333\r\n");
            UART_Send_String_w55("This is dpj received 333333\r\n");
            result = 1;
            // UART_Send_String_w55("Executing control command 3...\r\n");
            // result = execute_control3_w55();
            break;

        default:
            UART_Send_String_w55("This is dpj received ERROR！\r\n");
            
            UART_Send_String_w55("This is dpj received ERROR！\r\n");
            
            UART_Send_String_w55("This is dpj received ERROR！\r\n");
            // UART_Send_String_w55("Unknown command\r\n");
            result = 0;
            break;
    }

    // ?? 发送执行结果回后端
    send_result_to_backend_w55(command, result);

    return result;
}


/**
 * @brief 接收并处理后端发送的控制指令
 * @return 处理结果: 1-成功, 0-失败
 */
unsigned char receive_control_command_w55(void)
{
    unsigned char recv_buf[20];        // 接收缓冲区
    unsigned int recv_len = 0;         // 接收数据长度
    unsigned char socket_status;       // Socket状态
    unsigned char command;             // 控制指令
    unsigned char i;                   // 循环变量

    // ?? 检查Socket状态
    socket_status = Get_Socket_Status_w55(0);

    if(socket_status == SOCK_ESTABLISHED_w55) {
        // ?? 尝试接收来自后端的数据
        recv_len = TCP_Recv_Data_w55(0, recv_buf, 20);

        if(recv_len > 0) {
            // ?? 添加字符串结束符
            if(recv_len < 20) {
                recv_buf[recv_len] = '\0';
            } else {
                recv_buf[19] = '\0';
            }

            // ?? 查找有效的控制指令
            for(i = 0; i < recv_len; i++) {
                command = recv_buf[i];

                // ?? 检查是否为有效控制指令
                if(command == '0' || command == '1' || command == '2' || command == '3') {

                    // ?? 串口输出收到的指令（用于调试）
                    UART_Send_String_w55("Received command from backend: ");
                    UART_Send_Char_w55(command);
                    UART_Send_String_w55("\r\n");

                    // ?? 处理控制指令
                    if(process_control_command_w55(command)) {
                        return 1;  // 成功处理
                    }
                }
            }

            // ?? 如果没有找到有效指令，输出调试信息
            UART_Send_String_w55("Invalid command received from backend\r\n");
        }
    }

    return 0;
}



//#region W5500基础函数
// W5500延时函数
void delay_ms_w55(unsigned int ms)
{
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 1000; j++);
}


// SPI写一个字节
void SPI_Write_Byte_w55(unsigned char dat)
{
    unsigned char i;
    for(i = 0; i < 8; i++)
    {
        W5500_MOSI = (dat & 0x80) ? 1 : 0;
        dat <<= 1;
        W5500_SCLK = 1;
        W5500_SCLK = 0;
    }
}

// SPI读一个字节
unsigned char SPI_Read_Byte_w55(void)
{
    unsigned char i, dat = 0;
    for(i = 0; i < 8; i++)
    {
        dat <<= 1;
        W5500_SCLK = 1;
        if(W5500_MISO) dat |= 0x01;
        W5500_SCLK = 0;
    }
    return dat;
}
//#endregion

//#region W5500寄存器操作函数
// W5500写寄存器
void W5500_Write_Reg_w55(unsigned int addr, unsigned char dat)
{
    unsigned char control_byte;
    unsigned int real_addr = addr;
    
    // 判断寄存器类型并设置控制字节
    if(addr < 0x0008) {
        // 通用寄存器 (0x0000-0x0007)
        control_byte = 0x04;  // 写命令，VDM=1，RWB=0，OM=00
    } else if(addr >= 0x0008 && addr < 0x0108) {
        // Socket 0寄存器 (0x0008-0x0107)
        control_byte = 0x0C;  // Socket 0寄存器写命令 (BSB=00, RWB=0, OM=01)
        real_addr = addr - 0x0008;  // 转换为Socket内偏移地址
    } else if(addr >= 0x0108 && addr < 0x0208) {
        // Socket 1寄存器 (0x0108-0x0207)
        control_byte = 0x2C;  // Socket 1寄存器写命令 (BSB=01, RWB=0, OM=01)
        real_addr = addr - 0x0108;  // 转换为Socket内偏移地址
    } else {
        // 其他寄存器，使用通用寄存器访问
        control_byte = 0x04;
    }
    
    W5500_SCS = 0;
    SPI_Write_Byte_w55(real_addr >> 8);     // 地址高字节
    SPI_Write_Byte_w55(real_addr & 0xFF);   // 地址低字节
    SPI_Write_Byte_w55(control_byte);       // 控制字节
    SPI_Write_Byte_w55(dat);                // 数据
    W5500_SCS = 1;
}

// W5500读寄存器
unsigned char W5500_Read_Reg_w55(unsigned int addr)
{
    unsigned char dat;
    unsigned char control_byte;
    unsigned int real_addr = addr;
    
    // 判断寄存器类型并设置控制字节
    if(addr < 0x0008) {
        // 通用寄存器 (0x0000-0x0007)
        control_byte = 0x00;  // 读命令，VDM=1，RWB=1，OM=00
    } else if(addr >= 0x0008 && addr < 0x0108) {
        // Socket 0寄存器 (0x0008-0x0107)
        control_byte = 0x08;  // Socket 0寄存器读命令 (BSB=00, RWB=1, OM=01)
        real_addr = addr - 0x0008;  // 转换为Socket内偏移地址
    } else if(addr >= 0x0108 && addr < 0x0208) {
        // Socket 1寄存器 (0x0108-0x0207)
        control_byte = 0x28;  // Socket 1寄存器读命令 (BSB=01, RWB=1, OM=01)
        real_addr = addr - 0x0108;  // 转换为Socket内偏移地址
    } else {
        // 其他寄存器，使用通用寄存器访问
        control_byte = 0x00;
    }
    
    W5500_SCS = 0;
    SPI_Write_Byte_w55(real_addr >> 8);     // 地址高字节
    SPI_Write_Byte_w55(real_addr & 0xFF);   // 地址低字节
    SPI_Write_Byte_w55(control_byte);       // 控制字节
    dat = SPI_Read_Byte_w55();              // 读取数据
    W5500_SCS = 1;
    return dat;
}

// W5500写缓冲区
void W5500_Write_Buffer_w55(unsigned int addr, unsigned char cb, unsigned char *buf, unsigned int len)
{
    unsigned int i;
    W5500_SCS = 0;
    SPI_Write_Byte_w55(addr >> 8);     // 地址高字节
    SPI_Write_Byte_w55(addr & 0xFF);   // 地址低字节
    SPI_Write_Byte_w55(cb);            // 控制字节
    for(i = 0; i < len; i++)
    {
        SPI_Write_Byte_w55(buf[i]);
    }
    W5500_SCS = 1;
}

// W5500读缓冲区
void W5500_Read_Buffer_w55(unsigned int addr, unsigned char cb, unsigned char *buf, unsigned int len)
{
    unsigned int i;
    W5500_SCS = 0;
    SPI_Write_Byte_w55(addr >> 8);     // 地址高字节
    SPI_Write_Byte_w55(addr & 0xFF);   // 地址低字节
    SPI_Write_Byte_w55(cb);            // 控制字节
    for(i = 0; i < len; i++)
    {
        buf[i] = SPI_Read_Byte_w55();
    }
    W5500_SCS = 1;
}

// W5500硬件复位
void W5500_HW_Reset_w55(void)
{
    W5500_RST = 0;
    delay_ms_w55(50);
    W5500_RST = 1;
    delay_ms_w55(200);
}
//#endregion

//#region W5500初始化和网络配置函数
// W5500初始化
void W5500_Init_w55(void)
{
    // 硬件复位
    W5500_HW_Reset_w55();
    
    // 设置网关地址
    W5500_Write_Buffer_w55(W5500_GAR, 0x04, gateway_w55, 4);
    
    // 设置子网掩码
    W5500_Write_Buffer_w55(W5500_SUBR, 0x04, subnet_w55, 4);
    
    // 设置MAC地址
    W5500_Write_Buffer_w55(W5500_SHAR, 0x04, mac_addr_w55, 6);
    
    // 设置IP地址
    W5500_Write_Buffer_w55(W5500_SIPR, 0x04, local_ip_w55, 4);
    
    // 设置重传时间和次数
    W5500_Write_Reg_w55(W5500_RTR, 0x07);     // 重传时间
    W5500_Write_Reg_w55(W5500_RTR + 1, 0xD0);
    W5500_Write_Reg_w55(W5500_RCR, 8);        // 重传次数
}
//#endregion

//#region W5500 Socket操作函数
// Socket初始化
unsigned char Socket_Init_w55(unsigned char socket, unsigned char mode, unsigned int port)
{
    unsigned int socket_base = 0x0008 + socket * 0x0100;
    unsigned int timeout = 0;
    unsigned char status;
    
    // 首先关闭Socket（如果已打开）
    W5500_Write_Reg_w55(socket_base + Sn_CR, SOCK_CLOSE_w55);
    delay_ms_w55(10);
    
    // 设置Socket模式
    W5500_Write_Reg_w55(socket_base + Sn_MR, mode);
    
    // 设置端口
    W5500_Write_Reg_w55(socket_base + Sn_PORT, port >> 8);
    W5500_Write_Reg_w55(socket_base + Sn_PORT + 1, port & 0xFF);
    
    // 打开Socket
    UART_Send_String_w55("Attempting to open socket...\r\n");
    W5500_Write_Reg_w55(socket_base + Sn_CR, SOCK_OPEN_w55);
    UART_Send_String_w55("Socket open command sent, waiting for CR to clear...\r\n");
    
    // 等待命令寄存器清零，添加超时机制
    timeout = 0;
    while(W5500_Read_Reg_w55(socket_base + Sn_CR) && timeout < 1000) {
        delay_ms_w55(1);
        timeout++;
    }
    
    if(timeout >= 1000) {
        UART_Send_String_w55("Socket open timeout!\r\n");
        return 0;
    }
    
    // 检查Socket状态
    status = W5500_Read_Reg_w55(socket_base + Sn_SR);
    if(status == SOCK_INIT_w55) {
        UART_Send_String_w55("Socket opened successfully!\r\n");
        return 1;
    } else {
        UART_Send_String_w55("Socket open failed, status: 0x");
        // 发送状态的十六进制值
        UART_Send_Char_w55((status >> 4) < 10 ? (status >> 4) + '0' : (status >> 4) - 10 + 'A');
        UART_Send_Char_w55((status & 0x0F) < 10 ? (status & 0x0F) + '0' : (status & 0x0F) - 10 + 'A');
        UART_Send_String_w55("\r\n");
        return 0;
    }
}



// TCP服务器监听
unsigned char TCP_Server_Listen_w55(unsigned char socket)
{
    unsigned int socket_base = 0x0008 + socket * 0x0100;
    unsigned int timeout = 0;
    unsigned char status;
    
    // 检查Socket状态是否为INIT
    status = W5500_Read_Reg_w55(socket_base + Sn_SR);
    if(status != SOCK_INIT_w55) {
        UART_Send_String_w55("Socket not in INIT state, cannot listen\r\n");
        return 0;
    }
    
    // 发送监听命令
    W5500_Write_Reg_w55(socket_base + Sn_CR, SOCK_LISTEN_w55);
    
    // 等待命令寄存器清零，添加超时机制
    timeout = 0;
    while(W5500_Read_Reg_w55(socket_base + Sn_CR) && timeout < 1000) {
        delay_ms_w55(1);
        timeout++;
    }
    
    if(timeout >= 1000) {
        UART_Send_String_w55("Listen command timeout!\r\n");
        return 0;
    }
    
    // 检查Socket状态是否变为LISTEN
    status = W5500_Read_Reg_w55(socket_base + Sn_SR);
    if(status == SOCK_LISTENING_w55) {
        UART_Send_String_w55("Socket listening successfully!\r\n");
        return 1;
    } else {
        UART_Send_String_w55("Listen failed, status: 0x");
        UART_Send_Char_w55((status >> 4) < 10 ? (status >> 4) + '0' : (status >> 4) - 10 + 'A');
        UART_Send_Char_w55((status & 0x0F) < 10 ? (status & 0x0F) + '0' : (status & 0x0F) - 10 + 'A');
        UART_Send_String_w55("\r\n");
        return 0;
    }
}

// 获取Socket状态
unsigned char Get_Socket_Status_w55(unsigned char socket)
{
    unsigned int socket_base = 0x0008 + socket * 0x0100;
    return W5500_Read_Reg_w55(socket_base + Sn_SR);
}

// TCP发送数据
void TCP_Send_Data_w55(unsigned char socket, unsigned char *buf, unsigned int len)
{
    unsigned int socket_base = 0x0008 + socket * 0x0100;
    unsigned int tx_wr;
    
    // 获取发送写指针
    tx_wr = W5500_Read_Reg_w55(socket_base + Sn_TX_WR) << 8;
    tx_wr |= W5500_Read_Reg_w55(socket_base + Sn_TX_WR + 1);
    
    // 写入发送缓冲区
    W5500_Write_Buffer_w55(0x8000 + socket * 0x1000 + (tx_wr & 0x0FFF), (socket << 5) | 0x14, buf, len);
    
    // 更新发送写指针
    tx_wr += len;
    W5500_Write_Reg_w55(socket_base + Sn_TX_WR, tx_wr >> 8);
    W5500_Write_Reg_w55(socket_base + Sn_TX_WR + 1, tx_wr & 0xFF);
    
    // 发送数据
    W5500_Write_Reg_w55(socket_base + Sn_CR, SOCK_SEND_w55);
    while(W5500_Read_Reg_w55(socket_base + Sn_CR));
}

// TCP接收数据
unsigned int TCP_Recv_Data_w55(unsigned char socket, unsigned char *buf, unsigned int max_len)
{
    unsigned int socket_base = 0x0008 + socket * 0x0100;
    unsigned int rx_len, rx_rd;
    
    // 获取接收数据长度
    rx_len = W5500_Read_Reg_w55(socket_base + Sn_RX_RSR) << 8;
    rx_len |= W5500_Read_Reg_w55(socket_base + Sn_RX_RSR + 1);
    
    if(rx_len == 0) return 0;
    
    // 限制接收长度，防止缓冲区溢出
    if(rx_len > max_len) rx_len = max_len;
    
    // 获取接收读指针
    rx_rd = W5500_Read_Reg_w55(socket_base + Sn_RX_RD) << 8;
    rx_rd |= W5500_Read_Reg_w55(socket_base + Sn_RX_RD + 1);
    
    // 读取接收缓冲区
    W5500_Read_Buffer_w55(0xC000 + socket * 0x1000 + (rx_rd & 0x0FFF), (socket << 5) | 0x18, buf, rx_len);
    
    // 更新接收读指针
    rx_rd += rx_len;
    W5500_Write_Reg_w55(socket_base + Sn_RX_RD, rx_rd >> 8);
    W5500_Write_Reg_w55(socket_base + Sn_RX_RD + 1, rx_rd & 0xFF);
    
    // 发送接收命令
    W5500_Write_Reg_w55(socket_base + Sn_CR, SOCK_RECV_w55);
    while(W5500_Read_Reg_w55(socket_base + Sn_CR));
    
    return rx_len;
}
//#endregion


// W5500串口发送字符
void UART_Send_Char_w55(unsigned char c)
{
    SBUF = c;
    while(!TI);
    TI = 0;
}

// W5500串口发送字符串
void UART_Send_String_w55(unsigned char *str)
{
    while(*str)
    {
        UART_Send_Char_w55(*str++);
    }
}
//#endregion

//#region RC522延时函数
// RC522延时函数 - 适配11.0592MHz晶振
void delay_ms_rc(unsigned int ms) {
    unsigned int i, j;
    for(i = 0; i < ms; i++) {
        for(j = 0; j < 111; j++);  // 从120改为111，适配11.0592MHz
    }
}

void delay_us_rc(unsigned int us) {
    unsigned int i;
    for(i = 0; i < us; i++) {
        _nop_();_nop_();_nop_();_nop_();_nop_();
        _nop_();_nop_();_nop_();_nop_();_nop_();
    }
}
//#endregion

//#region RC522 SPI通信函数
// RC522 SPI写字节
void RC522_WriteByte_rc(unsigned char dat) {
    unsigned char i;
    RC522_SCK = 0;
    delay_us_rc(10);
    
    for(i = 0; i < 8; i++) {
        RC522_MOSI = (dat & 0x80) ? 1 : 0;
        delay_us_rc(10);
        RC522_SCK = 1;
        delay_us_rc(20);
        dat <<= 1;
        RC522_SCK = 0;
        delay_us_rc(10);
    }
}

// RC522 SPI读字节
unsigned char RC522_ReadByte_rc(void) {
    unsigned char i, dat;
    dat = 0;
    RC522_SCK = 0;
    delay_us_rc(10);
    
    for(i = 0; i < 8; i++) {
        dat <<= 1;
        RC522_SCK = 1;
        delay_us_rc(20);
        if(RC522_MISO) dat |= 0x01;
        RC522_SCK = 0;
        delay_us_rc(10);
    }
    return dat;
}
//#endregion

//#region RC522寄存器操作函数
// RC522写寄存器
void WriteReg_rc(unsigned char addr, unsigned char val) {
    RC522_SDA = 0;
    delay_us_rc(10);
    RC522_WriteByte_rc((addr << 1) & 0x7E);
    RC522_WriteByte_rc(val);
    delay_us_rc(10);
    RC522_SDA = 1;
    delay_us_rc(10);
}

// RC522读寄存器
unsigned char ReadReg_rc(unsigned char addr) {
    unsigned char val;
    RC522_SDA = 0;
    delay_us_rc(10);
    RC522_WriteByte_rc(((addr << 1) & 0x7E) | 0x80);
    val = RC522_ReadByte_rc();
    delay_us_rc(10);
    RC522_SDA = 1;
    delay_us_rc(10);
    return val;
}

// RC522设置位掩码
void SetBitMask_rc(unsigned char reg, unsigned char mask) {
    unsigned char tmp;
    tmp = ReadReg_rc(reg);
    WriteReg_rc(reg, tmp | mask);
}

// RC522清除位掩码
void ClearBitMask_rc(unsigned char reg, unsigned char mask) {
    unsigned char tmp;
    tmp = ReadReg_rc(reg);
    WriteReg_rc(reg, tmp & (~mask));
}
//#endregion

//#region RC522初始化函数
// RC522初始化
void RC522_Init_rc(void) {
    // 硬件复位
    RC522_RST = 0;
    delay_ms_rc(100);
    RC522_RST = 1;
    delay_ms_rc(500);
    
    // 软件复位
    WriteReg_rc(0x01, 0x0F);  // CommandReg - SoftReset
    delay_ms_rc(100);
    while(ReadReg_rc(0x01) & 0x10);  // 等待复位完成
    
    // 基本配置
    WriteReg_rc(0x14, 0x83);  // TxControlReg - 启用TX1和TX2
    WriteReg_rc(0x26, 0x7F);  // RFCfgReg - 最大增益
    WriteReg_rc(0x24, 0x26);  // ModWidthReg - 调制宽度
    WriteReg_rc(0x15, 0x40);  // TxASKReg - 100% ASK调制
    WriteReg_rc(0x11, 0x3D);  // ModeReg - CRC初始值0x6363
    WriteReg_rc(0x2A, 0x80);  // TModeReg - 自动启动定时器
    WriteReg_rc(0x2B, 0xA9);  // TPrescalerReg
    WriteReg_rc(0x2C, 0x03);  // TReloadRegH
    WriteReg_rc(0x2D, 0xFF);  // TReloadRegL
}
//#endregion

//#region RC522核心通信函数
// RC522通信函数
unsigned char RC522_Communicate_rc(unsigned char command, unsigned char *sendData, unsigned char sendLen, unsigned char *backData, unsigned int *backLen) {
    unsigned char status = 0;
    unsigned char irqEn = 0x00;
    unsigned char waitIRq = 0x00;
    unsigned char lastBits, n, i;
    unsigned int timeout = 0;
    
    switch(command) {
        case 0x0C:  // Transceive
            irqEn = 0x77;
            waitIRq = 0x30;
            break;
        default:
            break;
    }
    
    WriteReg_rc(0x02, irqEn | 0x80);    // CommIEnReg - 中断使能
    ClearBitMask_rc(0x04, 0x80);        // ComIrqReg - 清除中断请求位
    SetBitMask_rc(0x0A, 0x80);          // FIFOLevelReg - 清空FIFO
    WriteReg_rc(0x01, 0x00);            // CommandReg - 空闲状态
    
    // 向FIFO写入数据
    for(i = 0; i < sendLen; i++) {
        WriteReg_rc(0x09, sendData[i]);
    }
    
    // 执行命令
    WriteReg_rc(0x01, command);
    if(command == 0x0C) {
        SetBitMask_rc(0x0D, 0x80);      // BitFramingReg - StartSend=1
    }
    
    // 等待接收数据完成
    do {
        n = ReadReg_rc(0x04);           // ComIrqReg
        timeout++;
        if(timeout > 5000) {  // 从2000增加到5000
            status = 2;  // 超时
            break;
        }
        delay_us_rc(50);
    } while(!(n & 0x01) && !(n & waitIRq));
    
    ClearBitMask_rc(0x0D, 0x80);        // BitFramingReg - StartSend=0
    
    if(timeout < 2000) {
        if(!(ReadReg_rc(0x06) & 0x1B)) {  // ErrorReg - 无缓冲区溢出、奇偶校验错误、协议错误
            status = 0;  // 成功
            if(n & irqEn & 0x01) {
                status = 1;  // 未检测到卡片
            }
            
            if(command == 0x0C) {
                n = ReadReg_rc(0x0A);     // FIFOLevelReg
                lastBits = ReadReg_rc(0x0C) & 0x07;  // ControlReg
                if(lastBits) {
                    *backLen = (n - 1) * 8 + lastBits;
                } else {
                    *backLen = n * 8;
                }
                
                if(n == 0) {
                    n = 1;
                }
                if(n > 16) {
                    n = 16;
                }
                
                // 从FIFO读取接收到的数据
                for(i = 0; i < n; i++) {
                    backData[i] = ReadReg_rc(0x09);
                }
            }
        } else {
            status = 3;  // 错误
        }
    }
    
    return status;
}
//#endregion

//#region RC522卡片操作函数
// 寻卡
unsigned char RC522_Request_rc(unsigned char reqMode, unsigned char *TagType) {
    unsigned char status;
    unsigned int backBits;
    
    WriteReg_rc(0x0D, 0x07);    // BitFramingReg - TxLastBits = BitFramingReg[2..0]
    TagType[0] = reqMode;
    status = RC522_Communicate_rc(0x0C, TagType, 1, TagType, &backBits);
    
    if((status != 0) || (backBits != 0x10)) {
        status = 1;
    }
    
    return status;
}

// 防冲突，读取卡序列号
unsigned char RC522_Anticoll_rc(unsigned char *serNum) {
    unsigned char status;
    unsigned char i;
    unsigned char serNumCheck = 0;
    unsigned int unLen;
    
    WriteReg_rc(0x0D, 0x00);    // BitFramingReg - 清除位定义
    serNum[0] = 0x93;        // 防冲突命令
    serNum[1] = 0x20;
    status = RC522_Communicate_rc(0x0C, serNum, 2, serNum, &unLen);
    
    if(status == 0) {
        // 校验卡序列号
        for(i = 0; i < 4; i++) {
            serNumCheck ^= serNum[i];
        }
        if(serNumCheck != serNum[i]) {
            status = 1;
        }
    }
    
    return status;
}

// 选择卡片
unsigned char RC522_SelectTag_rc(unsigned char *serNum) {
    unsigned char status;
    unsigned char size;
    unsigned int recvBits;
    unsigned char buffer[9];
    
    buffer[0] = 0x93;  // SELECT命令
    buffer[1] = 0x70;
    for(size = 0; size < 5; size++) {
        buffer[size + 2] = *(serNum + size);
    }
    
    status = RC522_Communicate_rc(0x0C, buffer, 7, buffer, &recvBits);
    
    if((status == 0) && (recvBits == 0x18)) {
        size = buffer[0];
    } else {
        size = 0;
    }
    
    return size;
}

// RC522状态检查函数
unsigned char RC522_CheckStatus_rc(void) {
    unsigned char status;
    
    // 读取状态寄存器
    status = ReadReg_rc(0x06);  // ErrorReg
    if(status & 0x1B) {  // 检查错误位
        // 有错误，重新初始化
        RC522_Init_rc();
        return 0;
    }
    
    return 1;  // 状态正常
}

// RC522读卡并通过W5500发送UID的函数
unsigned char ReadCardAndSendUID_rc(void)
{
    unsigned char status;
    unsigned char str[16];         // 存储卡片类型信息
    unsigned char serNum[5];       // 存储卡片序列号(4字节UID + 1字节校验)
    unsigned char i,retry;         // 循环变量和重试计数器
    unsigned char uid_string[64];  // 存储格式化后的UID字符串
    unsigned char uid_index = 0;   // 字符串索引指针

    // 在开始读卡前检查RC522状态
    if(!RC522_CheckStatus_rc()) { // 检查RC522模块是否正常工作
        delay_ms_rc(100);
        return 0;
    }
    
    // 添加重试机制，最多重试3次
    for(retry = 0; retry < 3; retry++) {
        // 寻卡
        status = RC522_Request_rc(0x52, str);
        if(status != 0) {
            delay_ms_rc(10);  // 失败后短暂延时
            continue;
        }
        
        // 防冲突
        status = RC522_Anticoll_rc(serNum);
        if(status != 0) {
            delay_ms_rc(10);
            continue;
        }
        
        // 选择卡片
        status = RC522_SelectTag_rc(serNum);
        if(status > 0) {
            delay_ms_rc(10);
            continue;
        }
        
        // 成功读取，跳出重试循环
        break;
    }
    
    // 如果重试3次都失败，返回0
    if(retry >= 3) {
        return 0;
    }
    
    // 构建UID字符串
    uid_string[uid_index++] = 'U';
    uid_string[uid_index++] = 'I';
    uid_string[uid_index++] = 'D';
    uid_string[uid_index++] = ':';
    uid_string[uid_index++] = ' ';
    
    for(i = 0; i < 4; i++) {
        unsigned char temp;
        // 转换高4位
        temp = serNum[i] >> 4;
        uid_string[uid_index++] = (temp < 10) ? (temp + '0') : (temp - 10 + 'A');
        // 转换低4位
        temp = serNum[i] & 0x0F;
        uid_string[uid_index++] = (temp < 10) ? (temp + '0') : (temp - 10 + 'A');
        
        if(i < 3) {
            uid_string[uid_index++] = ' '; // 添加空格分隔
        }
    }
    
    uid_string[uid_index++] = '\r';
    uid_string[uid_index++] = '\n';
    uid_string[uid_index] = '\0'; // 字符串结束符
    
    // 通过W5500发送UID
    TCP_Send_Data_w55(0, uid_string, uid_index);
    
    // 同时通过串口 输出UID
    UART_Send_String_w55("Card detected, UID sent: ");
    UART_Send_String_w55(uid_string);
    
    return 1; // 成功读取并发送
}

// 整合的主函数 - W5500作为TCP服务器，同时读取RC522卡片
void main_integrated(void)
{
    unsigned char recv_buf[64]; // 接收数据
    unsigned int scan_count = 0; // 每隔一段时间扫描卡片
        
    // 统一串口初始化
    SCON = 0x50;      // 串口工作模式1（8位数据，可变波特率）
    TMOD &= 0x0F;     // 清除定时器1的模式位（保留定时器0设置）
    TMOD |= 0x20;     // 设置定时器1为模式2（8位自动重装）
    TH1 = 0xFD;       // 设置波特率为9600（11.0592MHz晶振）
    TL1 = 0xFD;       // 初值与重装值相同
    PCON &= 0x7F;     // 波特率不倍速
    TR1 = 1;          // 启动定时器1
    TI = 1;           // 设置发送中断标志（允许发送）
    RI = 0;           // 清除接收中断标志
    REN = 1;          // 允许串口接收
    
    
    // W5500以太网模块初始化
    W5500_Init_w55();    // 配置网络参数（IP、MAC、网关等）
    delay_ms_w55(1000);  // 等待W5500稳定

    // RC522 RFID读卡器初始化
    RC522_Init_rc();     // 配置RF参数和寄存器
    delay_ms_w55(500);   // 等待RC522稳定
        
    // 创建TCP服务器Socket
    Socket_Init_w55(0, 0x01, 5000);  // Socket0, TCP模式, 端口5000

    // 开始监听连接
    TCP_Server_Listen_w55(0);

    UART_Send_String_w55("All init is ok...\r\n");
    UART_Send_String_w55("Server listening on port 5000...\r\n");
    
    while(1)
    {
        unsigned char status = Get_Socket_Status_w55(0); // 读取Socket 0的当前状态 -通过SPI读取W5500的Socket状态寄存器(Sn_SR)
        
        if(status == SOCK_ESTABLISHED_w55)
        {
            // ?? 接收并处理来自后端的控制指令
            receive_control_command_w55();
            /*
            // 尝试接收数据
            unsigned int recv_len = TCP_Recv_Data_w55(0, recv_buf, 100);
            if(recv_len > 0)
            {
                // 回显接收到的数据
                TCP_Send_Data_w55(0, recv_buf, recv_len);
                UART_Send_String_w55("Data echoed to client\r\n");
            }
            */
            // 每隔一段时间扫描卡片
            scan_count++;
            if(scan_count % 5 == 0) { // 扫描频率
                if(ReadCardAndSendUID_rc()) { // 读到数据后重置
                    scan_count = 0; // 重置计数器
                    delay_ms_w55(1500); // 从1000增加到1500，读取成功后等待更长时间
                }
            }
        }
        else if(status == SOCK_CLOSE_WAIT_w55) 
        {
            // 客户端请求关闭连接
            W5500_Write_Reg_w55(0x0008 + Sn_CR, SOCK_DISCON_w55);
            UART_Send_String_w55("Client disconnected\r\n");
        }
        else if(status == SOCK_CLOSED_w55)
        {
            // Socket已关闭，重新初始化并监听
            UART_Send_String_w55("Socket closed, restarting...\r\n");
            if(Socket_Init_w55(0, 0x01, 5000)) {
                if(TCP_Server_Listen_w55(0)) {
                    UART_Send_String_w55("Server restarted successfully\r\n");
                } else {
                    UART_Send_String_w55("Failed to restart listening\r\n");
                }
            } else {
                UART_Send_String_w55("Failed to restart socket\r\n");
            }
        }
        
        delay_ms_w55(50); // 减少延时，提高响应速度
    }
}

// 默认主函数 - 可以选择运行不同的功能
void main(void)
{
     main_integrated();
}

