#include <reg52.h>
#include <intrins.h>

//#region W5500�Ĵ�����ַ����
// W5500�Ĵ�����ַ����
#define W5500_MR        0x0000  // ģʽ�Ĵ���
#define W5500_GAR       0x0001  // ���ص�ַ�Ĵ���
#define W5500_SUBR      0x0005  // ��������Ĵ���
#define W5500_SHAR      0x0009  // ԴMAC��ַ�Ĵ���
#define W5500_SIPR      0x000F  // ԴIP��ַ�Ĵ���
#define W5500_RTR       0x0019  // �ش���ʱ�Ĵ���
#define W5500_RCR       0x001B  // �ش������Ĵ���

// Socket�Ĵ���ƫ��
#define Sn_MR           0x0000  // Socketģʽ�Ĵ���
#define Sn_CR           0x0001  // Socket����Ĵ���
#define Sn_IR           0x0002  // Socket�жϼĴ���
#define Sn_SR           0x0003  // Socket״̬�Ĵ���
#define Sn_PORT         0x0004  // Socket�˿ڼĴ���
#define Sn_DHAR         0x0006  // Ŀ��MAC��ַ�Ĵ���
#define Sn_DIPR         0x000C  // Ŀ��IP��ַ�Ĵ���
#define Sn_DPORT        0x0010  // Ŀ��˿ڼĴ���
#define Sn_MSSR         0x0012  // Socket���δ�С�Ĵ���
#define Sn_TOS          0x0015  // Socket�������ͼĴ���
#define Sn_TTL          0x0016  // Socket����ʱ��Ĵ���
#define Sn_RXBUF_SIZE   0x001E  // Socket���ջ�������С�Ĵ���
#define Sn_TXBUF_SIZE   0x001F  // Socket���ͻ�������С�Ĵ���
#define Sn_TX_FSR       0x0020  // Socket���Ϳ��д�С�Ĵ���
#define Sn_TX_RD        0x0022  // Socket���Ͷ�ָ��Ĵ���
#define Sn_TX_WR        0x0024  // Socket����дָ��Ĵ���
#define Sn_RX_RSR       0x0026  // Socket���մ�С�Ĵ���
#define Sn_RX_RD        0x0028  // Socket���ն�ָ��Ĵ���
#define Sn_RX_WR        0x002A  // Socket����дָ��Ĵ���
//#endregion

//#region W5500 Socket�����״̬����
// Socket����
#define SOCK_OPEN_w55       0x01
#define SOCK_LISTEN_w55     0x02
#define SOCK_CONNECT_w55    0x04
#define SOCK_DISCON_w55     0x08
#define SOCK_CLOSE_w55      0x10
#define SOCK_SEND_w55       0x20
#define SOCK_SEND_MAC_w55   0x21
#define SOCK_SEND_KEEP_w55  0x22
#define SOCK_RECV_w55       0x40

// Socket״̬
#define SOCK_CLOSED_w55     0x00
#define SOCK_INIT_w55       0x13
#define SOCK_LISTENING_w55  0x14  // �޸���������SOCK_LISTEN�����ͻ
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

//#region RC522���Ŷ���
// RC522���Ŷ���
sbit RC522_SDA  = P1^0;
sbit RC522_SCK  = P1^1;
sbit RC522_MOSI = P1^2;
sbit RC522_MISO = P1^3;
sbit RC522_RST  = P1^4;
//#endregion

//#region W5500���Ŷ���
// W5500���Ŷ���
sbit W5500_SCLK = P2^0;  // SPIʱ��
sbit W5500_SCS  = P2^1;  // SPIƬѡ
sbit W5500_MOSI = P2^2;  // ��������
sbit W5500_MISO = P2^3;  // ����ӳ�
sbit W5500_INT  = P2^4;  // �ж����� - δʹ��
sbit W5500_RST  = P2^5;  // ��λ����
//#endregion

//#region W5500�������ñ���
// �������� - �����������绷������
// ����W5500��IP��ַ�������ĵ�����ͬһ����
unsigned char local_ip_w55[4] = {192, 168, 31, 56};     // W5500��IP��ַ
unsigned char subnet_w55[4] = {255, 255, 255, 0};        // �������룬����������һ��
unsigned char gateway_w55[4] = {192, 168, 31, 1};      // ���ص�ַ��������Ĭ������һ��
unsigned char mac_addr_w55[6] = {0x00, 0x08, 0xDC, 0x11, 0x11, 0x11}; // MAC��ַ

//#region ����ָ���
// ����ָ���
#define CMD_ERROR    '0'
#define CMD_CONTROL1 '1'
#define CMD_CONTROL2 '2'
#define CMD_CONTROL3 '3'

// ����״̬�ṹ��
typedef struct {
    unsigned char device_status;      // �豸״̬
    unsigned char control1_active;    // ����1״̬
    unsigned char control2_active;    // ����2״̬
    unsigned char control3_active;    // ����3״̬
    unsigned int last_command_time;   // ���ָ��ʱ��(��Ϊ16λ)
} ControlStatus;

//==============================================================================
// ȫ�ֱ���
//==============================================================================
ControlStatus control_status = {0, 0, 0, 0, 0};

// ���ڳ�ʼ����־λ
bit uart_initialized = 0;

//==============================================================================
// ��������
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
 * @brief ���˷���ָ��ִ�н��
 * @param command: ԭʼָ��
 * @param result: ִ�н�� (1-�ɹ�, 0-ʧ��)
 */
void send_result_to_backend_w55(unsigned char command, unsigned char result)
{
    unsigned char response[16];  // ��Ӧ������
    unsigned char response_len = 0;  // ��Ӧ����

    if (result) {
        // ?? �ɹ���Ӧ��"OK:X"
        response[response_len++] = 'O';
        response[response_len++] = 'K';
        response[response_len++] = ':';
        response[response_len++] = command;
    } else {
        // ?? ʧ����Ӧ��"ERR:X"
        response[response_len++] = 'E';
        response[response_len++] = 'R';
        response[response_len++] = 'R';
        response[response_len++] = ':';
        response[response_len++] = command;
    }

    response[response_len++] = '\r';
    response[response_len++] = '\n';

    // ?? ͨ��W5500������Ӧ�����
    if (Get_Socket_Status_w55(0) == SOCK_ESTABLISHED_w55) {
        TCP_Send_Data_w55(0, response, response_len);
        UART_Send_String_w55("Result sent to backend: ");
        response[response_len] = '\0';
        UART_Send_String_w55(response);
    }
}

/**
 * @brief ִ�и�λָ��
 */
unsigned char execute_reset_w55(void)
{
    // ?? ��λ�豸״̬
    P3 = 0x00;  // ����������

    UART_Send_String_w55("Device reset completed\r\n");
    return 1;
}

/**
 * @brief ִ�п���ָ��1
 */
unsigned char execute_control1_w55(void)
{
    // ?? ��������ʵ�������޸�
    // P3_0 = 1;  // ����ĳ���豸

    UART_Send_String_w55("Control 1 executed\r\n");
    return 1;
}

/**
 * @brief ִ�п���ָ��2
 */
unsigned char execute_control2_w55(void)
{
    // ?? ��������ʵ�������޸�
    // P3_1 = 1;  // ������һ���豸

    UART_Send_String_w55("Control 2 executed\r\n");
    return 1;
}

/**
 * @brief ִ�п���ָ��3
 */
unsigned char execute_control3_w55(void)
{
    // ?? ״̬��ѯ����������
    unsigned char status = P1;  // ��ȡ״̬

    UART_Send_String_w55("Control 3 executed - Status: 0x");
    UART_Send_Char_w55((status >> 4) < 10 ? (status >> 4) + '0' : (status >> 4) - 10 + 'A');
    UART_Send_Char_w55((status & 0x0F) < 10 ? (status & 0x0F) + '0' : (status & 0x0F) - 10 + 'A');
    UART_Send_String_w55("\r\n");

    return 1;
}

/**
 * @brief ������յ��Ŀ���ָ��
 * @param command: ����ָ���ַ� ('0', '1', '2', '3')
 * @return ������: 1-�ɹ�, 0-ʧ��
 */
unsigned char process_control_command_w55(unsigned char command)
{
    unsigned char result = 0;

    switch (command) {
        case '0':    // ������/��λ
            UART_Send_String_w55("This is dpj received 00000000000000000\r\n");    
            UART_Send_String_w55("This is dpj received 00000000000000000\r\n");            
            UART_Send_String_w55("This is dpj received 00000000000000000\r\n");                        
            result = 1;
            // UART_Send_String_w55("Executing reset command...\r\n");
            // result = execute_reset_w55();
            break;

        case '1':    // ����ָ��1
            UART_Send_String_w55("This is dpj received 1111111\r\n");    
            UART_Send_String_w55("This is dpj received 1111111\r\n");
            UART_Send_String_w55("This is dpj received 1111111\r\n");    
            result = 1;        
            // UART_Send_String_w55("Executing control command 1...\r\n");
            // result = execute_control1_w55();
            break;

        case '2':    // ����ָ��2
            UART_Send_String_w55("This is dpj received 22222\r\n");
            UART_Send_String_w55("This is dpj received 22222\r\n");
            UART_Send_String_w55("This is dpj received 22222\r\n");
            result = 1;
            // UART_Send_String_w55("Executing control command 2...\r\n");
            // result = execute_control2_w55();
            break;

        case '3':    // ����ָ��3
            UART_Send_String_w55("This is dpj received 333333\r\n");
            UART_Send_String_w55("This is dpj received 333333\r\n");
            UART_Send_String_w55("This is dpj received 333333\r\n");
            result = 1;
            // UART_Send_String_w55("Executing control command 3...\r\n");
            // result = execute_control3_w55();
            break;

        default:
            UART_Send_String_w55("This is dpj received ERROR��\r\n");
            
            UART_Send_String_w55("This is dpj received ERROR��\r\n");
            
            UART_Send_String_w55("This is dpj received ERROR��\r\n");
            // UART_Send_String_w55("Unknown command\r\n");
            result = 0;
            break;
    }

    // ?? ����ִ�н���غ��
    send_result_to_backend_w55(command, result);

    return result;
}


/**
 * @brief ���ղ������˷��͵Ŀ���ָ��
 * @return ������: 1-�ɹ�, 0-ʧ��
 */
unsigned char receive_control_command_w55(void)
{
    unsigned char recv_buf[20];        // ���ջ�����
    unsigned int recv_len = 0;         // �������ݳ���
    unsigned char socket_status;       // Socket״̬
    unsigned char command;             // ����ָ��
    unsigned char i;                   // ѭ������

    // ?? ���Socket״̬
    socket_status = Get_Socket_Status_w55(0);

    if(socket_status == SOCK_ESTABLISHED_w55) {
        // ?? ���Խ������Ժ�˵�����
        recv_len = TCP_Recv_Data_w55(0, recv_buf, 20);

        if(recv_len > 0) {
            // ?? ����ַ���������
            if(recv_len < 20) {
                recv_buf[recv_len] = '\0';
            } else {
                recv_buf[19] = '\0';
            }

            // ?? ������Ч�Ŀ���ָ��
            for(i = 0; i < recv_len; i++) {
                command = recv_buf[i];

                // ?? ����Ƿ�Ϊ��Ч����ָ��
                if(command == '0' || command == '1' || command == '2' || command == '3') {

                    // ?? ��������յ���ָ����ڵ��ԣ�
                    UART_Send_String_w55("Received command from backend: ");
                    UART_Send_Char_w55(command);
                    UART_Send_String_w55("\r\n");

                    // ?? �������ָ��
                    if(process_control_command_w55(command)) {
                        return 1;  // �ɹ�����
                    }
                }
            }

            // ?? ���û���ҵ���Чָ����������Ϣ
            UART_Send_String_w55("Invalid command received from backend\r\n");
        }
    }

    return 0;
}



//#region W5500��������
// W5500��ʱ����
void delay_ms_w55(unsigned int ms)
{
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 1000; j++);
}


// SPIдһ���ֽ�
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

// SPI��һ���ֽ�
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

//#region W5500�Ĵ�����������
// W5500д�Ĵ���
void W5500_Write_Reg_w55(unsigned int addr, unsigned char dat)
{
    unsigned char control_byte;
    unsigned int real_addr = addr;
    
    // �жϼĴ������Ͳ����ÿ����ֽ�
    if(addr < 0x0008) {
        // ͨ�üĴ��� (0x0000-0x0007)
        control_byte = 0x04;  // д���VDM=1��RWB=0��OM=00
    } else if(addr >= 0x0008 && addr < 0x0108) {
        // Socket 0�Ĵ��� (0x0008-0x0107)
        control_byte = 0x0C;  // Socket 0�Ĵ���д���� (BSB=00, RWB=0, OM=01)
        real_addr = addr - 0x0008;  // ת��ΪSocket��ƫ�Ƶ�ַ
    } else if(addr >= 0x0108 && addr < 0x0208) {
        // Socket 1�Ĵ��� (0x0108-0x0207)
        control_byte = 0x2C;  // Socket 1�Ĵ���д���� (BSB=01, RWB=0, OM=01)
        real_addr = addr - 0x0108;  // ת��ΪSocket��ƫ�Ƶ�ַ
    } else {
        // �����Ĵ�����ʹ��ͨ�üĴ�������
        control_byte = 0x04;
    }
    
    W5500_SCS = 0;
    SPI_Write_Byte_w55(real_addr >> 8);     // ��ַ���ֽ�
    SPI_Write_Byte_w55(real_addr & 0xFF);   // ��ַ���ֽ�
    SPI_Write_Byte_w55(control_byte);       // �����ֽ�
    SPI_Write_Byte_w55(dat);                // ����
    W5500_SCS = 1;
}

// W5500���Ĵ���
unsigned char W5500_Read_Reg_w55(unsigned int addr)
{
    unsigned char dat;
    unsigned char control_byte;
    unsigned int real_addr = addr;
    
    // �жϼĴ������Ͳ����ÿ����ֽ�
    if(addr < 0x0008) {
        // ͨ�üĴ��� (0x0000-0x0007)
        control_byte = 0x00;  // �����VDM=1��RWB=1��OM=00
    } else if(addr >= 0x0008 && addr < 0x0108) {
        // Socket 0�Ĵ��� (0x0008-0x0107)
        control_byte = 0x08;  // Socket 0�Ĵ��������� (BSB=00, RWB=1, OM=01)
        real_addr = addr - 0x0008;  // ת��ΪSocket��ƫ�Ƶ�ַ
    } else if(addr >= 0x0108 && addr < 0x0208) {
        // Socket 1�Ĵ��� (0x0108-0x0207)
        control_byte = 0x28;  // Socket 1�Ĵ��������� (BSB=01, RWB=1, OM=01)
        real_addr = addr - 0x0108;  // ת��ΪSocket��ƫ�Ƶ�ַ
    } else {
        // �����Ĵ�����ʹ��ͨ�üĴ�������
        control_byte = 0x00;
    }
    
    W5500_SCS = 0;
    SPI_Write_Byte_w55(real_addr >> 8);     // ��ַ���ֽ�
    SPI_Write_Byte_w55(real_addr & 0xFF);   // ��ַ���ֽ�
    SPI_Write_Byte_w55(control_byte);       // �����ֽ�
    dat = SPI_Read_Byte_w55();              // ��ȡ����
    W5500_SCS = 1;
    return dat;
}

// W5500д������
void W5500_Write_Buffer_w55(unsigned int addr, unsigned char cb, unsigned char *buf, unsigned int len)
{
    unsigned int i;
    W5500_SCS = 0;
    SPI_Write_Byte_w55(addr >> 8);     // ��ַ���ֽ�
    SPI_Write_Byte_w55(addr & 0xFF);   // ��ַ���ֽ�
    SPI_Write_Byte_w55(cb);            // �����ֽ�
    for(i = 0; i < len; i++)
    {
        SPI_Write_Byte_w55(buf[i]);
    }
    W5500_SCS = 1;
}

// W5500��������
void W5500_Read_Buffer_w55(unsigned int addr, unsigned char cb, unsigned char *buf, unsigned int len)
{
    unsigned int i;
    W5500_SCS = 0;
    SPI_Write_Byte_w55(addr >> 8);     // ��ַ���ֽ�
    SPI_Write_Byte_w55(addr & 0xFF);   // ��ַ���ֽ�
    SPI_Write_Byte_w55(cb);            // �����ֽ�
    for(i = 0; i < len; i++)
    {
        buf[i] = SPI_Read_Byte_w55();
    }
    W5500_SCS = 1;
}

// W5500Ӳ����λ
void W5500_HW_Reset_w55(void)
{
    W5500_RST = 0;
    delay_ms_w55(50);
    W5500_RST = 1;
    delay_ms_w55(200);
}
//#endregion

//#region W5500��ʼ�����������ú���
// W5500��ʼ��
void W5500_Init_w55(void)
{
    // Ӳ����λ
    W5500_HW_Reset_w55();
    
    // �������ص�ַ
    W5500_Write_Buffer_w55(W5500_GAR, 0x04, gateway_w55, 4);
    
    // ������������
    W5500_Write_Buffer_w55(W5500_SUBR, 0x04, subnet_w55, 4);
    
    // ����MAC��ַ
    W5500_Write_Buffer_w55(W5500_SHAR, 0x04, mac_addr_w55, 6);
    
    // ����IP��ַ
    W5500_Write_Buffer_w55(W5500_SIPR, 0x04, local_ip_w55, 4);
    
    // �����ش�ʱ��ʹ���
    W5500_Write_Reg_w55(W5500_RTR, 0x07);     // �ش�ʱ��
    W5500_Write_Reg_w55(W5500_RTR + 1, 0xD0);
    W5500_Write_Reg_w55(W5500_RCR, 8);        // �ش�����
}
//#endregion

//#region W5500 Socket��������
// Socket��ʼ��
unsigned char Socket_Init_w55(unsigned char socket, unsigned char mode, unsigned int port)
{
    unsigned int socket_base = 0x0008 + socket * 0x0100;
    unsigned int timeout = 0;
    unsigned char status;
    
    // ���ȹر�Socket������Ѵ򿪣�
    W5500_Write_Reg_w55(socket_base + Sn_CR, SOCK_CLOSE_w55);
    delay_ms_w55(10);
    
    // ����Socketģʽ
    W5500_Write_Reg_w55(socket_base + Sn_MR, mode);
    
    // ���ö˿�
    W5500_Write_Reg_w55(socket_base + Sn_PORT, port >> 8);
    W5500_Write_Reg_w55(socket_base + Sn_PORT + 1, port & 0xFF);
    
    // ��Socket
    UART_Send_String_w55("Attempting to open socket...\r\n");
    W5500_Write_Reg_w55(socket_base + Sn_CR, SOCK_OPEN_w55);
    UART_Send_String_w55("Socket open command sent, waiting for CR to clear...\r\n");
    
    // �ȴ�����Ĵ������㣬��ӳ�ʱ����
    timeout = 0;
    while(W5500_Read_Reg_w55(socket_base + Sn_CR) && timeout < 1000) {
        delay_ms_w55(1);
        timeout++;
    }
    
    if(timeout >= 1000) {
        UART_Send_String_w55("Socket open timeout!\r\n");
        return 0;
    }
    
    // ���Socket״̬
    status = W5500_Read_Reg_w55(socket_base + Sn_SR);
    if(status == SOCK_INIT_w55) {
        UART_Send_String_w55("Socket opened successfully!\r\n");
        return 1;
    } else {
        UART_Send_String_w55("Socket open failed, status: 0x");
        // ����״̬��ʮ������ֵ
        UART_Send_Char_w55((status >> 4) < 10 ? (status >> 4) + '0' : (status >> 4) - 10 + 'A');
        UART_Send_Char_w55((status & 0x0F) < 10 ? (status & 0x0F) + '0' : (status & 0x0F) - 10 + 'A');
        UART_Send_String_w55("\r\n");
        return 0;
    }
}



// TCP����������
unsigned char TCP_Server_Listen_w55(unsigned char socket)
{
    unsigned int socket_base = 0x0008 + socket * 0x0100;
    unsigned int timeout = 0;
    unsigned char status;
    
    // ���Socket״̬�Ƿ�ΪINIT
    status = W5500_Read_Reg_w55(socket_base + Sn_SR);
    if(status != SOCK_INIT_w55) {
        UART_Send_String_w55("Socket not in INIT state, cannot listen\r\n");
        return 0;
    }
    
    // ���ͼ�������
    W5500_Write_Reg_w55(socket_base + Sn_CR, SOCK_LISTEN_w55);
    
    // �ȴ�����Ĵ������㣬��ӳ�ʱ����
    timeout = 0;
    while(W5500_Read_Reg_w55(socket_base + Sn_CR) && timeout < 1000) {
        delay_ms_w55(1);
        timeout++;
    }
    
    if(timeout >= 1000) {
        UART_Send_String_w55("Listen command timeout!\r\n");
        return 0;
    }
    
    // ���Socket״̬�Ƿ��ΪLISTEN
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

// ��ȡSocket״̬
unsigned char Get_Socket_Status_w55(unsigned char socket)
{
    unsigned int socket_base = 0x0008 + socket * 0x0100;
    return W5500_Read_Reg_w55(socket_base + Sn_SR);
}

// TCP��������
void TCP_Send_Data_w55(unsigned char socket, unsigned char *buf, unsigned int len)
{
    unsigned int socket_base = 0x0008 + socket * 0x0100;
    unsigned int tx_wr;
    
    // ��ȡ����дָ��
    tx_wr = W5500_Read_Reg_w55(socket_base + Sn_TX_WR) << 8;
    tx_wr |= W5500_Read_Reg_w55(socket_base + Sn_TX_WR + 1);
    
    // д�뷢�ͻ�����
    W5500_Write_Buffer_w55(0x8000 + socket * 0x1000 + (tx_wr & 0x0FFF), (socket << 5) | 0x14, buf, len);
    
    // ���·���дָ��
    tx_wr += len;
    W5500_Write_Reg_w55(socket_base + Sn_TX_WR, tx_wr >> 8);
    W5500_Write_Reg_w55(socket_base + Sn_TX_WR + 1, tx_wr & 0xFF);
    
    // ��������
    W5500_Write_Reg_w55(socket_base + Sn_CR, SOCK_SEND_w55);
    while(W5500_Read_Reg_w55(socket_base + Sn_CR));
}

// TCP��������
unsigned int TCP_Recv_Data_w55(unsigned char socket, unsigned char *buf, unsigned int max_len)
{
    unsigned int socket_base = 0x0008 + socket * 0x0100;
    unsigned int rx_len, rx_rd;
    
    // ��ȡ�������ݳ���
    rx_len = W5500_Read_Reg_w55(socket_base + Sn_RX_RSR) << 8;
    rx_len |= W5500_Read_Reg_w55(socket_base + Sn_RX_RSR + 1);
    
    if(rx_len == 0) return 0;
    
    // ���ƽ��ճ��ȣ���ֹ���������
    if(rx_len > max_len) rx_len = max_len;
    
    // ��ȡ���ն�ָ��
    rx_rd = W5500_Read_Reg_w55(socket_base + Sn_RX_RD) << 8;
    rx_rd |= W5500_Read_Reg_w55(socket_base + Sn_RX_RD + 1);
    
    // ��ȡ���ջ�����
    W5500_Read_Buffer_w55(0xC000 + socket * 0x1000 + (rx_rd & 0x0FFF), (socket << 5) | 0x18, buf, rx_len);
    
    // ���½��ն�ָ��
    rx_rd += rx_len;
    W5500_Write_Reg_w55(socket_base + Sn_RX_RD, rx_rd >> 8);
    W5500_Write_Reg_w55(socket_base + Sn_RX_RD + 1, rx_rd & 0xFF);
    
    // ���ͽ�������
    W5500_Write_Reg_w55(socket_base + Sn_CR, SOCK_RECV_w55);
    while(W5500_Read_Reg_w55(socket_base + Sn_CR));
    
    return rx_len;
}
//#endregion


// W5500���ڷ����ַ�
void UART_Send_Char_w55(unsigned char c)
{
    SBUF = c;
    while(!TI);
    TI = 0;
}

// W5500���ڷ����ַ���
void UART_Send_String_w55(unsigned char *str)
{
    while(*str)
    {
        UART_Send_Char_w55(*str++);
    }
}
//#endregion

//#region RC522��ʱ����
// RC522��ʱ���� - ����11.0592MHz����
void delay_ms_rc(unsigned int ms) {
    unsigned int i, j;
    for(i = 0; i < ms; i++) {
        for(j = 0; j < 111; j++);  // ��120��Ϊ111������11.0592MHz
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

//#region RC522 SPIͨ�ź���
// RC522 SPIд�ֽ�
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

// RC522 SPI���ֽ�
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

//#region RC522�Ĵ�����������
// RC522д�Ĵ���
void WriteReg_rc(unsigned char addr, unsigned char val) {
    RC522_SDA = 0;
    delay_us_rc(10);
    RC522_WriteByte_rc((addr << 1) & 0x7E);
    RC522_WriteByte_rc(val);
    delay_us_rc(10);
    RC522_SDA = 1;
    delay_us_rc(10);
}

// RC522���Ĵ���
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

// RC522����λ����
void SetBitMask_rc(unsigned char reg, unsigned char mask) {
    unsigned char tmp;
    tmp = ReadReg_rc(reg);
    WriteReg_rc(reg, tmp | mask);
}

// RC522���λ����
void ClearBitMask_rc(unsigned char reg, unsigned char mask) {
    unsigned char tmp;
    tmp = ReadReg_rc(reg);
    WriteReg_rc(reg, tmp & (~mask));
}
//#endregion

//#region RC522��ʼ������
// RC522��ʼ��
void RC522_Init_rc(void) {
    // Ӳ����λ
    RC522_RST = 0;
    delay_ms_rc(100);
    RC522_RST = 1;
    delay_ms_rc(500);
    
    // �����λ
    WriteReg_rc(0x01, 0x0F);  // CommandReg - SoftReset
    delay_ms_rc(100);
    while(ReadReg_rc(0x01) & 0x10);  // �ȴ���λ���
    
    // ��������
    WriteReg_rc(0x14, 0x83);  // TxControlReg - ����TX1��TX2
    WriteReg_rc(0x26, 0x7F);  // RFCfgReg - �������
    WriteReg_rc(0x24, 0x26);  // ModWidthReg - ���ƿ��
    WriteReg_rc(0x15, 0x40);  // TxASKReg - 100% ASK����
    WriteReg_rc(0x11, 0x3D);  // ModeReg - CRC��ʼֵ0x6363
    WriteReg_rc(0x2A, 0x80);  // TModeReg - �Զ�������ʱ��
    WriteReg_rc(0x2B, 0xA9);  // TPrescalerReg
    WriteReg_rc(0x2C, 0x03);  // TReloadRegH
    WriteReg_rc(0x2D, 0xFF);  // TReloadRegL
}
//#endregion

//#region RC522����ͨ�ź���
// RC522ͨ�ź���
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
    
    WriteReg_rc(0x02, irqEn | 0x80);    // CommIEnReg - �ж�ʹ��
    ClearBitMask_rc(0x04, 0x80);        // ComIrqReg - ����ж�����λ
    SetBitMask_rc(0x0A, 0x80);          // FIFOLevelReg - ���FIFO
    WriteReg_rc(0x01, 0x00);            // CommandReg - ����״̬
    
    // ��FIFOд������
    for(i = 0; i < sendLen; i++) {
        WriteReg_rc(0x09, sendData[i]);
    }
    
    // ִ������
    WriteReg_rc(0x01, command);
    if(command == 0x0C) {
        SetBitMask_rc(0x0D, 0x80);      // BitFramingReg - StartSend=1
    }
    
    // �ȴ������������
    do {
        n = ReadReg_rc(0x04);           // ComIrqReg
        timeout++;
        if(timeout > 5000) {  // ��2000���ӵ�5000
            status = 2;  // ��ʱ
            break;
        }
        delay_us_rc(50);
    } while(!(n & 0x01) && !(n & waitIRq));
    
    ClearBitMask_rc(0x0D, 0x80);        // BitFramingReg - StartSend=0
    
    if(timeout < 2000) {
        if(!(ReadReg_rc(0x06) & 0x1B)) {  // ErrorReg - �޻������������żУ�����Э�����
            status = 0;  // �ɹ�
            if(n & irqEn & 0x01) {
                status = 1;  // δ��⵽��Ƭ
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
                
                // ��FIFO��ȡ���յ�������
                for(i = 0; i < n; i++) {
                    backData[i] = ReadReg_rc(0x09);
                }
            }
        } else {
            status = 3;  // ����
        }
    }
    
    return status;
}
//#endregion

//#region RC522��Ƭ��������
// Ѱ��
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

// ����ͻ����ȡ�����к�
unsigned char RC522_Anticoll_rc(unsigned char *serNum) {
    unsigned char status;
    unsigned char i;
    unsigned char serNumCheck = 0;
    unsigned int unLen;
    
    WriteReg_rc(0x0D, 0x00);    // BitFramingReg - ���λ����
    serNum[0] = 0x93;        // ����ͻ����
    serNum[1] = 0x20;
    status = RC522_Communicate_rc(0x0C, serNum, 2, serNum, &unLen);
    
    if(status == 0) {
        // У�鿨���к�
        for(i = 0; i < 4; i++) {
            serNumCheck ^= serNum[i];
        }
        if(serNumCheck != serNum[i]) {
            status = 1;
        }
    }
    
    return status;
}

// ѡ��Ƭ
unsigned char RC522_SelectTag_rc(unsigned char *serNum) {
    unsigned char status;
    unsigned char size;
    unsigned int recvBits;
    unsigned char buffer[9];
    
    buffer[0] = 0x93;  // SELECT����
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

// RC522״̬��麯��
unsigned char RC522_CheckStatus_rc(void) {
    unsigned char status;
    
    // ��ȡ״̬�Ĵ���
    status = ReadReg_rc(0x06);  // ErrorReg
    if(status & 0x1B) {  // ������λ
        // �д������³�ʼ��
        RC522_Init_rc();
        return 0;
    }
    
    return 1;  // ״̬����
}

// RC522������ͨ��W5500����UID�ĺ���
unsigned char ReadCardAndSendUID_rc(void)
{
    unsigned char status;
    unsigned char str[16];         // �洢��Ƭ������Ϣ
    unsigned char serNum[5];       // �洢��Ƭ���к�(4�ֽ�UID + 1�ֽ�У��)
    unsigned char i,retry;         // ѭ�����������Լ�����
    unsigned char uid_string[64];  // �洢��ʽ�����UID�ַ���
    unsigned char uid_index = 0;   // �ַ�������ָ��

    // �ڿ�ʼ����ǰ���RC522״̬
    if(!RC522_CheckStatus_rc()) { // ���RC522ģ���Ƿ���������
        delay_ms_rc(100);
        return 0;
    }
    
    // ������Ի��ƣ��������3��
    for(retry = 0; retry < 3; retry++) {
        // Ѱ��
        status = RC522_Request_rc(0x52, str);
        if(status != 0) {
            delay_ms_rc(10);  // ʧ�ܺ������ʱ
            continue;
        }
        
        // ����ͻ
        status = RC522_Anticoll_rc(serNum);
        if(status != 0) {
            delay_ms_rc(10);
            continue;
        }
        
        // ѡ��Ƭ
        status = RC522_SelectTag_rc(serNum);
        if(status > 0) {
            delay_ms_rc(10);
            continue;
        }
        
        // �ɹ���ȡ����������ѭ��
        break;
    }
    
    // �������3�ζ�ʧ�ܣ�����0
    if(retry >= 3) {
        return 0;
    }
    
    // ����UID�ַ���
    uid_string[uid_index++] = 'U';
    uid_string[uid_index++] = 'I';
    uid_string[uid_index++] = 'D';
    uid_string[uid_index++] = ':';
    uid_string[uid_index++] = ' ';
    
    for(i = 0; i < 4; i++) {
        unsigned char temp;
        // ת����4λ
        temp = serNum[i] >> 4;
        uid_string[uid_index++] = (temp < 10) ? (temp + '0') : (temp - 10 + 'A');
        // ת����4λ
        temp = serNum[i] & 0x0F;
        uid_string[uid_index++] = (temp < 10) ? (temp + '0') : (temp - 10 + 'A');
        
        if(i < 3) {
            uid_string[uid_index++] = ' '; // ��ӿո�ָ�
        }
    }
    
    uid_string[uid_index++] = '\r';
    uid_string[uid_index++] = '\n';
    uid_string[uid_index] = '\0'; // �ַ���������
    
    // ͨ��W5500����UID
    TCP_Send_Data_w55(0, uid_string, uid_index);
    
    // ͬʱͨ������ ���UID
    UART_Send_String_w55("Card detected, UID sent: ");
    UART_Send_String_w55(uid_string);
    
    return 1; // �ɹ���ȡ������
}

// ���ϵ������� - W5500��ΪTCP��������ͬʱ��ȡRC522��Ƭ
void main_integrated(void)
{
    unsigned char recv_buf[64]; // ��������
    unsigned int scan_count = 0; // ÿ��һ��ʱ��ɨ�迨Ƭ
        
    // ͳһ���ڳ�ʼ��
    SCON = 0x50;      // ���ڹ���ģʽ1��8λ���ݣ��ɱ䲨���ʣ�
    TMOD &= 0x0F;     // �����ʱ��1��ģʽλ��������ʱ��0���ã�
    TMOD |= 0x20;     // ���ö�ʱ��1Ϊģʽ2��8λ�Զ���װ��
    TH1 = 0xFD;       // ���ò�����Ϊ9600��11.0592MHz����
    TL1 = 0xFD;       // ��ֵ����װֵ��ͬ
    PCON &= 0x7F;     // �����ʲ�����
    TR1 = 1;          // ������ʱ��1
    TI = 1;           // ���÷����жϱ�־�������ͣ�
    RI = 0;           // ��������жϱ�־
    REN = 1;          // �����ڽ���
    
    
    // W5500��̫��ģ���ʼ��
    W5500_Init_w55();    // �������������IP��MAC�����صȣ�
    delay_ms_w55(1000);  // �ȴ�W5500�ȶ�

    // RC522 RFID��������ʼ��
    RC522_Init_rc();     // ����RF�����ͼĴ���
    delay_ms_w55(500);   // �ȴ�RC522�ȶ�
        
    // ����TCP������Socket
    Socket_Init_w55(0, 0x01, 5000);  // Socket0, TCPģʽ, �˿�5000

    // ��ʼ��������
    TCP_Server_Listen_w55(0);

    UART_Send_String_w55("All init is ok...\r\n");
    UART_Send_String_w55("Server listening on port 5000...\r\n");
    
    while(1)
    {
        unsigned char status = Get_Socket_Status_w55(0); // ��ȡSocket 0�ĵ�ǰ״̬ -ͨ��SPI��ȡW5500��Socket״̬�Ĵ���(Sn_SR)
        
        if(status == SOCK_ESTABLISHED_w55)
        {
            // ?? ���ղ��������Ժ�˵Ŀ���ָ��
            receive_control_command_w55();
            /*
            // ���Խ�������
            unsigned int recv_len = TCP_Recv_Data_w55(0, recv_buf, 100);
            if(recv_len > 0)
            {
                // ���Խ��յ�������
                TCP_Send_Data_w55(0, recv_buf, recv_len);
                UART_Send_String_w55("Data echoed to client\r\n");
            }
            */
            // ÿ��һ��ʱ��ɨ�迨Ƭ
            scan_count++;
            if(scan_count % 5 == 0) { // ɨ��Ƶ��
                if(ReadCardAndSendUID_rc()) { // �������ݺ�����
                    scan_count = 0; // ���ü�����
                    delay_ms_w55(1500); // ��1000���ӵ�1500����ȡ�ɹ���ȴ�����ʱ��
                }
            }
        }
        else if(status == SOCK_CLOSE_WAIT_w55) 
        {
            // �ͻ�������ر�����
            W5500_Write_Reg_w55(0x0008 + Sn_CR, SOCK_DISCON_w55);
            UART_Send_String_w55("Client disconnected\r\n");
        }
        else if(status == SOCK_CLOSED_w55)
        {
            // Socket�ѹرգ����³�ʼ��������
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
        
        delay_ms_w55(50); // ������ʱ�������Ӧ�ٶ�
    }
}

// Ĭ�������� - ����ѡ�����в�ͬ�Ĺ���
void main(void)
{
     main_integrated();
}

