import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import serial
import serial.tools.list_ports
import threading
import time
import csv
import socket

class StudentManager:
    def __init__(self, filename='students.csv'):
        self.filename = filename
        self.students = {}  # key=uid, value=dict{name,id,class}
        self.load_students()

    def load_students(self):
        try:
            with open(self.filename, newline='', encoding='utf-8') as csvfile:
                reader = csv.DictReader(csvfile)
                for row in reader:
                    self.students[row['uid'].upper()] = {
                        'name': row['name'],
                        'id': row['id'],
                        'class': row['class']
                    }
        except FileNotFoundError:
            self.students = {}

    def save_students(self):
        with open(self.filename, 'w', newline='', encoding='utf-8') as csvfile:
            fieldnames = ['uid', 'name', 'id', 'class']
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            for uid, info in self.students.items():
                writer.writerow({
                    'uid': uid,
                    'name': info['name'],
                    'id': info['id'],
                    'class': info['class']
                })

    def add_student(self, uid, name, student_id, class_name):
        uid = uid.upper()
        if uid in self.students:
            return False
        self.students[uid] = {'name': name, 'id': student_id, 'class': class_name}
        self.save_students()
        return True

    def update_student(self, uid, name, student_id, class_name):
        uid = uid.upper()
        if uid not in self.students:
            return False
        self.students[uid] = {'name': name, 'id': student_id, 'class': class_name}
        self.save_students()
        return True

    def delete_student(self, uid):
        uid = uid.upper()
        if uid in self.students:
            del self.students[uid]
            self.save_students()
            return True
        return False

    def get_student(self, uid):
        return self.students.get(uid.upper())

class AttendanceApp:
    def __init__(self, master):
        self.master = master
        self.master.title("RFID考勤上位机")

        self.serial_port = None
        self.tcp_socket = None
        self.use_tcp = False
        self.running = False
        self.attendance_active = False
        self.countdown_time = 20 * 60
        self.countdown_running = False
        self.countdown_thread = None

        self.attendance_records = []  # 格式：{'uid','name','id','class','time'}
        self.student_manager = StudentManager()

        self.current_students = []
        self.current_student_index = 0
        self.display_loop_running = False

        self.create_widgets()
        self.refresh_serial_ports()

    def create_widgets(self):
        row = 0
        # 通信方式选择
        ttk.Label(self.master, text="通信方式:").grid(row=row, column=0, sticky='w', padx=5)
        self.combobox_comm = ttk.Combobox(self.master, values=["串口", "TCP"], state="readonly")
        self.combobox_comm.current(0)
        self.combobox_comm.grid(row=row, column=1, padx=5, sticky='w')
        self.combobox_comm.bind("<<ComboboxSelected>>", self.comm_mode_changed)
        row += 1

        # 串口选择
        ttk.Label(self.master, text="串口:").grid(row=row, column=0, sticky='w', padx=5)
        self.combobox_serial = ttk.Combobox(self.master, values=[], state="readonly")
        self.combobox_serial.grid(row=row, column=1, padx=5, sticky='w')
        self.refresh_serial_btn = ttk.Button(self.master, text="刷新串口", command=self.refresh_serial_ports)
        self.refresh_serial_btn.grid(row=row, column=2, padx=5)
        row += 1

        # TCP地址和端口
        ttk.Label(self.master, text="TCP地址:").grid(row=row, column=0, sticky='w', padx=5)
        self.entry_tcp_ip = ttk.Entry(self.master)
        self.entry_tcp_ip.insert(0, "127.0.0.1")
        self.entry_tcp_ip.grid(row=row, column=1, padx=5, sticky='w')
        ttk.Label(self.master, text="端口:").grid(row=row, column=2, sticky='w', padx=5)
        self.entry_tcp_port = ttk.Entry(self.master, width=6)
        self.entry_tcp_port.insert(0, "9000")
        self.entry_tcp_port.grid(row=row, column=3, sticky='w')
        row += 1

        # 连接按钮
        self.connect_btn = ttk.Button(self.master, text="连接", command=self.connect)
        self.connect_btn.grid(row=row, column=0, padx=5, pady=5)
        self.disconnect_btn = ttk.Button(self.master, text="断开", command=self.disconnect, state='disabled')
        self.disconnect_btn.grid(row=row, column=1, padx=5, pady=5)
        row += 1

        # 学生信息输入区域（UID + 姓名 + 学号 + 班级）
        ttk.Label(self.master, text="UID:").grid(row=row, column=0, sticky="w", padx=5)
        self.uid_entry = ttk.Entry(self.master)
        self.uid_entry.grid(row=row, column=1, padx=5, sticky='w')
        row += 1

        ttk.Label(self.master, text="姓名:").grid(row=row, column=0, sticky="w", padx=5)
        self.name_entry = ttk.Entry(self.master)
        self.name_entry.grid(row=row, column=1, padx=5, sticky='w')
        row += 1

        ttk.Label(self.master, text="学号:").grid(row=row, column=0, sticky="w", padx=5)
        self.id_entry = ttk.Entry(self.master)
        self.id_entry.grid(row=row, column=1, padx=5, sticky='w')
        row += 1

        ttk.Label(self.master, text="班级:").grid(row=row, column=0, sticky="w", padx=5)
        self.class_entry = ttk.Entry(self.master)
        self.class_entry.grid(row=row, column=1, padx=5, sticky='w')
        row += 1

        # 写入卡片（保存学生信息）按钮
        self.write_btn = ttk.Button(self.master, text="写入卡片", command=self.write_card, state='disabled')
        self.write_btn.grid(row=row, column=0, padx=5, pady=5)

        # 删除卡片（删除学生信息）按钮
        self.delete_btn = ttk.Button(self.master, text="删除卡片", command=self.delete_card, state='disabled')
        self.delete_btn.grid(row=row, column=1, padx=5, pady=5)

        # 考勤控制按钮
        self.start_btn = ttk.Button(self.master, text="开始考勤", command=self.start_attendance, state='disabled')
        self.start_btn.grid(row=row, column=2, padx=5, pady=5)
        self.end_btn = ttk.Button(self.master, text="结束考勤", command=self.end_attendance, state='disabled')
        self.end_btn.grid(row=row, column=3, padx=5, pady=5)
        row += 1

        # 倒计时标签
        self.countdown_label = ttk.Label(self.master, text="")
        self.countdown_label.grid(row=row, column=0, columnspan=3)
        row += 1

        # 查看历史考勤和导出CSV按钮
        self.view_history_btn = ttk.Button(self.master, text="查看历史考勤", command=self.view_history, state='disabled')
        self.view_history_btn.grid(row=row, column=0, padx=5, pady=5)
        self.export_csv_btn = ttk.Button(self.master, text="导出CSV", command=self.export_csv, state='disabled')
        self.export_csv_btn.grid(row=row, column=1, padx=5, pady=5)
        row += 1

        # 打卡学生循环显示区域
        ttk.Label(self.master, text="当前打卡学生:").grid(row=row, column=0, sticky='w', padx=5)
        self.current_student_label = ttk.Label(self.master, text="", font=("Arial", 14))
        self.current_student_label.grid(row=row, column=1, columnspan=2, sticky='w', padx=5)
        row += 1

        # 日志显示区
        self.log_text = tk.Text(self.master, height=15, width=70)
        self.log_text.grid(row=row, column=0, columnspan=3, padx=5, pady=5)
        self.log_text.config(state='disabled')

        # 初始TCP输入禁用
        self.toggle_tcp_inputs(False)

    def toggle_tcp_inputs(self, enable):
        state = 'normal' if enable else 'disabled'
        self.entry_tcp_ip.config(state=state)
        self.entry_tcp_port.config(state=state)

    def comm_mode_changed(self, event=None):
        mode = self.combobox_comm.get()
        if mode == "串口":
            self.combobox_serial.config(state='readonly')
            self.refresh_serial_btn.config(state='normal')
            self.toggle_tcp_inputs(False)
            self.use_tcp = False
        else:
            self.combobox_serial.config(state='disabled')
            self.refresh_serial_btn.config(state='disabled')
            self.toggle_tcp_inputs(True)
            self.use_tcp = True

    def refresh_serial_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.combobox_serial['values'] = ports
        if ports:
            self.combobox_serial.current(0)

    def connect(self):
        if self.use_tcp:
            ip = self.entry_tcp_ip.get()
            try:
                port = int(self.entry_tcp_port.get())
            except:
                messagebox.showerror("错误", "端口格式错误")
                return
            try:
                self.tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.tcp_socket.connect((ip, port))
                self.running = True
                self.log(f"TCP连接成功 {ip}:{port}")
            except Exception as e:
                messagebox.showerror("错误", f"TCP连接失败: {e}")
                return
        else:
            port = self.combobox_serial.get()
            if not port:
                messagebox.showwarning("警告", "请选择串口")
                return
            try:
                self.serial_port = serial.Serial(port, baudrate=115200, timeout=0.5)
                self.running = True
                self.log(f"串口已打开 {port}")
            except Exception as e:
                messagebox.showerror("错误", f"串口打开失败: {e}")
                return

        self.connect_btn.config(state='disabled')
        self.disconnect_btn.config(state='normal')
        self.write_btn.config(state='normal')
        self.delete_btn.config(state='normal')
        self.start_btn.config(state='normal')
        self.end_btn.config(state='normal')
        self.view_history_btn.config(state='normal')
        self.export_csv_btn.config(state='normal')

        self.reader_thread = threading.Thread(target=self.read_from_device)
        self.reader_thread.daemon = True
        self.reader_thread.start()

    def disconnect(self):
        self.running = False
        self.attendance_active = False
        self.countdown_running = False
        self.display_loop_running = False

        try:
            if self.use_tcp and self.tcp_socket:
                self.tcp_socket.close()
                self.tcp_socket = None
            if not self.use_tcp and self.serial_port and self.serial_port.is_open:
                self.serial_port.close()
                self.serial_port = None
        except Exception as e:
            self.log(f"断开异常: {e}")

        self.connect_btn.config(state='normal')
        self.disconnect_btn.config(state='disabled')
        self.write_btn.config(state='disabled')
        self.start_btn.config(state='disabled')
        self.end_btn.config(state='disabled')
        self.view_history_btn.config(state='disabled')
        self.export_csv_btn.config(state='disabled')
        self.countdown_label.config(text="")
        self.current_student_label.config(text="")
        self.log("已断开连接")

    def send_data(self, data_bytes):
        try:
            if self.use_tcp and self.tcp_socket:
                self.tcp_socket.sendall(data_bytes)
            elif not self.use_tcp and self.serial_port and self.serial_port.is_open:
                self.serial_port.write(data_bytes)
            else:
                self.log("发送失败：未连接设备")
                return False
            return True
        except Exception as e:
            self.log(f"发送失败: {e}")
            return False

    def write_card(self):
        uid = self.uid_entry.get().strip().upper()
        name = self.name_entry.get().strip()
        student_id = self.id_entry.get().strip()
        class_name = self.class_entry.get().strip()

        if not uid or not name or not student_id or not class_name:
            messagebox.showwarning("警告", "请完整填写UID、姓名、学号和班级")
            return

        if not self.student_manager.add_student(uid, name, student_id, class_name):
            self.student_manager.update_student(uid, name, student_id, class_name)

        self.log(f"学生信息保存成功: UID[{uid}], 姓名[{name}], 学号[{student_id}], 班级[{class_name}]")

    def delete_card(self):
        uid = self.uid_entry.get().strip().upper()

        if not uid:
            messagebox.showwarning("警告", "请正确填写UID")
            return

        # 先查询是否有该学生
        student = self.student_manager.get_student(uid)
        if not student:
            messagebox.showinfo("提示", f"UID[{uid}] 对应的学生信息不存在")
            return

        # 删除学生
        success = self.student_manager.delete_student(uid)
        if success:
            self.log(f"学生信息删除成功")
            messagebox.showinfo("成功", "学生信息删除成功")
            # 清空输入框
            self.uid_entry.delete(0, 'end')
            self.name_entry.delete(0, 'end')
            self.id_entry.delete(0, 'end')
            self.class_entry.delete(0, 'end')
        else:
            messagebox.showerror("错误", "删除失败，请重试")


    def start_attendance(self):
        if self.attendance_active:
            messagebox.showwarning("警告", "考勤已在进行中")
            return
        if self.send_data(b"START"):
            self.log("考勤开始")
            self.attendance_active = True
            self.countdown_time = 20 * 60
            self.countdown_running = True
            self.current_students.clear()
            self.current_student_index = 0
            self.display_loop_running = True

            self.countdown_thread = threading.Thread(target=self.countdown)
            self.countdown_thread.daemon = True
            self.countdown_thread.start()

            self.display_loop_thread = threading.Thread(target=self.loop_display_students)
            self.display_loop_thread.daemon = True
            self.display_loop_thread.start()
        else:
            self.log("发送开始考勤失败")
    def countdown(self):
        while self.countdown_running and self.countdown_time > 0:
            mins, secs = divmod(self.countdown_time, 60)
            time_str = f"考勤倒计时：{mins:02d}:{secs:02d}"
            self.master.after(0, self.countdown_label.config, {'text': time_str})
            time.sleep(1)
            self.countdown_time -= 1

        if self.countdown_running and self.countdown_time == 0:
            self.master.after(0, self.log, "考勤倒计时结束，自动结束考勤")
            self.master.after(0, self.end_attendance)

    def end_attendance(self):
        if not self.attendance_active:
            messagebox.showinfo("提示", "考勤尚未开始")
            return
        if self.send_data(b"END"):
            self.log("考勤结束")
        self.attendance_active = False
        self.countdown_running = False
        self.display_loop_running = False
        self.countdown_label.config(text="")
        self.current_student_label.config(text="")

    def read_from_device(self):
        while self.running:
            try:
                if self.use_tcp and self.tcp_socket:
                    data = self.tcp_socket.recv(18)  # 每次读18字节UID数据包
                elif not self.use_tcp and self.serial_port and self.serial_port.is_open:
                    data = self.serial_port.read(18)
                else:
                    data = b''

                if data:
                    self.process_received_data(data)
                else:
                    time.sleep(0.1)
            except Exception as e:
                self.log(f"读取异常: {e}")
                time.sleep(1)

    def process_received_data(self, data):
        # 如果是 bytes，先转成字符串
        if isinstance(data, bytes):
            data_str = data.decode('ascii', errors='ignore')
        else:
            data_str = data

        if len(data_str) == 18 and data_str.startswith("UID: "):
            # 取出UID部分，去除头部"UID: "，去掉回车换行
            uid_part = data_str[5:].strip()  # "3A 7F B2 01"

            # 去掉所有空格
            uid_str = uid_part.replace(' ', '')  # "3A7FB201"

            self.log(f"收到UID字符串(无空格): {uid_str}")


            if self.attendance_active:
                student = self.student_manager.get_student(uid_str)
                if student:
                    if not any(s['uid'] == uid_str for s in self.current_students):
                        record = {
                            'uid': uid_str,
                            'name': student['name'],
                            'id': student['id'],
                            'class': student['class'],
                            'time': time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
                        }
                        self.attendance_records.append(record)
                        self.current_students.append(record)
                        print(f"考勤记录添加: {record}")
                        self.log(f"考勤记录添加: UID[{uid_str}], 姓名[{student['name']}], 学号[{student['id']}], 班级[{student['class']}]")
                    else:
                        self.log(f"重复打卡忽略: UID[{uid_str}] 姓名[{student['name']}]")
                else:
                    self.log(f"未知UID打卡: {uid_str}")
        else:
            self.log(f"收到无效数据: {data.hex()}")


    def loop_display_students(self):
        last_shown_uid = None
        while self.display_loop_running:
            if self.current_students:
                # 获取最后一个打卡记录
                record = self.current_students[-1]
                # 只有当最新学生和上次显示的不同，才更新显示
                if record['uid'] != last_shown_uid:
                    display_text = f"{record['name']} ({record['id']}) - {record['class']}"
                    self.master.after(0, self.current_student_label.config, {'text': display_text})
                    last_shown_uid = record['uid']
            else:
                self.master.after(0, self.current_student_label.config, {'text': "无打卡学生"})
                last_shown_uid = None
            time.sleep(2)


    def view_history(self):
        if not self.attendance_records:
            messagebox.showinfo("提示", "无历史考勤记录")
            return

        top = tk.Toplevel(self.master)
        top.title("历史考勤记录")

        columns = ('uid', 'name', 'id', 'class', 'time')
        tree = ttk.Treeview(top, columns=columns, show='headings')
        tree.heading('uid', text='UID')
        tree.heading('name', text='姓名')
        tree.heading('id', text='学号')
        tree.heading('class', text='班级')
        tree.heading('time', text='打卡时间')

        for rec in self.attendance_records:
            tree.insert('', tk.END, values=(rec['uid'], rec['name'], rec['id'], rec['class'], rec['time']))

        tree.pack(expand=True, fill='both')

    def export_csv(self):
        if not self.attendance_records:
            messagebox.showinfo("提示", "无考勤记录可导出")
            return
        filename = filedialog.asksaveasfilename(defaultextension=".csv", 
                                                filetypes=[("CSV 文件", "*.csv")])
        if not filename:
            return
        try:
            with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
                fieldnames = ['uid', 'name', 'id', 'class', 'time']
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
                writer.writeheader()
                for rec in self.attendance_records:
                    writer.writerow(rec)
            messagebox.showinfo("成功", f"考勤记录已导出到 {filename}")
        except Exception as e:
            messagebox.showerror("错误", f"导出失败: {e}")

    def log(self, msg):
        self.log_text.config(state='normal')
        self.log_text.insert(tk.END, msg + '\n')
        self.log_text.see(tk.END)
        self.log_text.config(state='disabled')


def main():
    root = tk.Tk()
    app = AttendanceApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()

