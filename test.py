import socket
import threading
import time
import random

class SimulatedMCU_TCP:
    def __init__(self, host='127.0.0.1', port=9000):
        self.host = host
        self.port = port
        self.server = None
        self.client_socket = None
        self.client_addr = None
        self.running = True
        self.sending_attendance = False
        self.send_thread = None

    def start_server(self):
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  # 避免端口占用问题
        self.server.bind((self.host, self.port))
        self.server.listen(1)
        print(f"模拟单片机TCP服务器启动，监听 {self.host}:{self.port} ...")
        self.client_socket, self.client_addr = self.server.accept()
        print(f"上位机连接：{self.client_addr}")
        self.listen()

    def listen(self):
        while self.running:
            try:
                data = self.client_socket.recv(1024)
                if not data:
                    print("客户端断开连接")
                    break
                self.process_command(data)
            except Exception as e:
                print(f"接收数据异常: {e}")
                break
            time.sleep(0.1)
        self.close()

    def process_command(self, data):
        print(f"收到上位机数据(HEX): {data.hex()}")

        if b'START' in data:
            print("收到开始考勤命令")
            if not self.sending_attendance:
                self.sending_attendance = True
                self.send_thread = threading.Thread(target=self.send_attendance_data)
                self.send_thread.start()

        elif b'END' in data:
            print("收到结束考勤命令")
            self.sending_attendance = False

        elif data.startswith(b'\xAA'):
            print("收到写卡命令数据包，模拟写卡成功")

    def send_attendance_data(self):
        uid_list = [
            b"UID: 01 02 03 04\r\n",
            b"UID: DE AD BE EF\r\n",
            b"UID: 12 34 56 78\r\n",
            b"UID: 11 22 33 44\r\n"
        ]
        idx = 0
        while self.sending_attendance:
            uid = uid_list[idx % len(uid_list)]  # 循环发送预设UID
            try:
                self.client_socket.sendall(uid)
                print(f"发送考勤UID数据包 (HEX): {uid.hex().upper()}")
            except Exception as e:
                print(f"发送失败: {e}")
                self.sending_attendance = False
                break
            idx += 1
            time.sleep(5)

    def close(self):
        self.running = False
        self.sending_attendance = False
        if self.client_socket:
            try:
                self.client_socket.close()
            except:
                pass
        if self.server:
            try:
                self.server.close()
            except:
                pass
        print("模拟单片机TCP服务器关闭")

def main():
    mcu = SimulatedMCU_TCP(host='127.0.0.1', port=9000)
    try:
        mcu.start_server()
    except KeyboardInterrupt:
        print("退出模拟单片机程序")
    finally:
        mcu.close()

if __name__ == "__main__":
    main()

