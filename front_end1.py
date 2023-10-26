#!/usr/bin/python3

import sqlite3
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator
import matplotlib.animation as animation
from datetime import datetime
import time
import sys
import getopt

major = 5
minor = 1
xsize = 50
ysize = 50
offset = 0
pause = 0
file = 'EBS.db'
review = 0

def onClick(event):
    global pause
    pause ^= True


def select_db(offset):

    global xsize
    lf_oil = []
    rf_oil = []
    lb_oil = []
    rb_oil = []
    f_air = []
    b_air = []
    e_speed = []
    c_speed = []
    date = []
    conn = sqlite3.connect(file)
    c = conn.cursor()

    cursor = c.execute(
        "SELECT DATE, LF_OIL, RF_OIL, LB_OIL, RB_OIL, F_AIR, B_AIR, E_SPEED, C_SPEED FROM PRESSURE"
        " ORDER BY DATE DESC"
        " LIMIT ?"
        " OFFSET ?", (xsize, offset))
    for row in cursor:
        date.append(datetime.strptime(row[0], '%Y-%m-%d %H:%M:%S.%f'))
        lf_oil.append(row[1])
        rf_oil.append(row[2])
        lb_oil.append(row[3])
        rb_oil.append(row[4])
        f_air.append(row[5])
        b_air.append(row[6])
        e_speed.append(row[7])
        c_speed.append(row[8])

    date.reverse()
    lf_oil.reverse()
    rf_oil.reverse()
    lb_oil.reverse()
    rb_oil.reverse()
    f_air.reverse()
    b_air.reverse()
    e_speed.reverse()
    c_speed.reverse()
    return (date, lf_oil, rf_oil, lb_oil, rb_oil, f_air, b_air, e_speed, c_speed)

def select_db_esc(offset):

    global xsize
    lf_oil = []
    rf_oil = []
    lb_oil = []
    rb_oil = []
    f_air = []
    b_air = []
    e_speed = []
    c_speed = []
    date = []
    conn = sqlite3.connect(file)
    c = conn.cursor()

    cursor = c.execute(
        "SELECT DATE, LF_OIL, RF_OIL, LB_OIL, RB_OIL, F_AIR, B_AIR, E_SPEED, C_SPEED FROM PRESSURE"
        " ORDER BY DATE ASC"
        " LIMIT ?"
        " OFFSET ?", (xsize, offset))
    for row in cursor:
        date.append(datetime.strptime(row[0], '%Y-%m-%d %H:%M:%S.%f'))
        lf_oil.append(row[1])
        rf_oil.append(row[2])
        lb_oil.append(row[3])
        rb_oil.append(row[4])
        f_air.append(row[5])
        b_air.append(row[6])
        e_speed.append(row[7])
        c_speed.append(row[8])

    return (date, lf_oil, rf_oil, lb_oil, rb_oil, f_air, b_air, e_speed, c_speed)

def drow():
    global major, minor, xsize, ysize, offset

    fig = plt.figure()
    fig.canvas.mpl_connect('button_press_event', onClick)
    ax = fig.add_subplot(1, 2, 1)

    (date, lf_oil, rf_oil, lb_oil, rb_oil, f_air, b_air, e_speed, c_speed) = select_db(offset)
    ax.plot(date, lf_oil, 's-', color='red', label="LF-OIL")  # s-:方形
    ax.plot(date, rf_oil, 's-', color='green', label="RF-OIL")  # o-:圆形
    ax.plot(date, lb_oil, 's-', color='blue', label="LB-OIL")  # o-:圆形
    ax.plot(date, rb_oil, 's-', color='steelblue', label="RB-OIL")  # o-:圆形
    ax.plot(date, f_air, 'o-', color='orange', label="F-AIR")  # o-:圆形
    ax.plot(date, b_air, 'o-', color='pink', label="B-AIR")  # o-:圆形

    bx = fig.add_subplot(1, 2, 2)

    bx.plot(date, e_speed, 's-', color='red', label="E_SPEED")  # s-:方形
    bx.plot(date, c_speed, 's-', color='green', label="C_SPEED")  # o-:圆形


    def animation_function(offset):
        global date, lf_oil, rf_oil, rb_oil, f_air, b_air, e_speed, c_speed
        if (pause == 0):
            (date, lf_oil, rf_oil, lb_oil, rb_oil, f_air, b_air, e_speed, c_speed) = select_db(offset)
            print(offset)
            ax.clear()
            bx.clear()

            ax.set_xlabel("date")
            ax.set_ylabel("pressure(bar)")

            y_major_locator = MultipleLocator(float(major))
            y_minor_locator = MultipleLocator(float(minor))  # 将x轴次刻度标签设置为5的倍数
            ax.yaxis.set_major_locator(y_major_locator)
            ax.yaxis.set_minor_locator(y_minor_locator)
            ax.set_ylim(0, ysize)

            ax.plot(date, lf_oil, 's-', color='red', label="LF-OIL")  # s-:方形
            ax.plot(date, rf_oil, 's-', color='green', label="RF-OIL")  # o-:圆形
            ax.plot(date, lb_oil, 's-', color='blue', label="LB-OIL")  # o-:圆形
            ax.plot(date, rb_oil, 's-', color='steelblue',label="RB-OIL")  # o-:圆形
            ax.plot(date, f_air, 'o-', color='orange', label="F-AIR")  # o-:圆形
            ax.plot(date, b_air, 'o-', color='pink', label="B-AIR")  # o-:圆形
            ax.legend(loc="upper left")

            bx.set_xlabel("date")
            bx.set_ylabel("speed(m/s)")

            y_major_locator = MultipleLocator(float(1))
            y_minor_locator = MultipleLocator(float(0.2))  # 将x轴次刻度标签设置为5的倍数
            bx.yaxis.set_major_locator(y_major_locator)
            bx.yaxis.set_minor_locator(y_minor_locator)
            bx.set_ylim(0, 20)

            bx.plot(date, e_speed, 's-', color='red', label="E_SPEED")  # s-:方形
            bx.plot(date, c_speed, 's-', color='green', label="C_SPEED")  # o-:圆形
            bx.legend(loc="upper left")

    ani = animation.FuncAnimation(
        fig=fig, func=animation_function, frames=1, interval=200)
    plt.show()

def drow2():
    global major, minor, xsize, ysize

    fig = plt.figure()
    fig.canvas.mpl_connect('button_press_event', onClick)
    ax = fig.add_subplot(1, 1, 1)
    ax2 = ax.twinx()
    ax2.set_ylabel('speed m/s')
    ax.set_ylabel('pressure bar')

    (date, lf_oil, rf_oil, lb_oil, rb_oil, f_air, b_air, e_speed, c_speed) = select_db_esc(offset)
    ax.plot(date, lf_oil, 's-', color='red', label="LF-OIL")  # s-:方形
    ax.plot(date, rf_oil, 's-', color='green', label="RF-OIL")  # o-:圆形
    ax.plot(date, lb_oil, 's-', color='blue', label="LB-OIL")  # o-:圆形
    ax.plot(date, rb_oil, 's-', color='steelblue', label="RB-OIL")  # o-:圆形
    ax.plot(date, f_air, 'o-', color='orange', label="F-AIR")  # o-:圆形
    ax.plot(date, b_air, 'o-', color='pink', label="B-AIR")  # o-:圆形
    
    ax2.plot(date, e_speed, 's-', color='yellow', label="E_SPEED m/s")  # s-:方形
    ax2.plot(date, c_speed, 's-', color='black', label="C_SPEED m/s")  # o-:圆形
    


    def animation_function(offset1):
        global date, lf_oil, rf_oil, rb_oil, f_air, b_air, e_speed, c_speed, offset
        
        if (pause == 0):
            (date, lf_oil, rf_oil, lb_oil, rb_oil, f_air, b_air, e_speed, c_speed) = select_db_esc(offset)
            ax.clear()
            ax2.clear()

            offset += 1
            
            # ax.set_xlabel("date")
            # ax2.set_ylabel("pressure(bar)")

            y_major_locator = MultipleLocator(float(major))
            y_minor_locator = MultipleLocator(float(minor))  # 将x轴次刻度标签设置为5的倍数
            ax.yaxis.set_major_locator(y_major_locator)
            ax.yaxis.set_minor_locator(y_minor_locator)

            y_major_locator1 = MultipleLocator(float(2))
            y_minor_locator1 = MultipleLocator(float(0.5))  # 将x轴次刻度标签设置为5的倍数
            ax2.yaxis.set_major_locator(y_major_locator1)
            ax2.yaxis.set_minor_locator(y_minor_locator1)

            ax.set_ylim(0, ysize)
            ax2.set_ylim(0, 20)

            ax.plot(date, lf_oil, 's-', color='red', label="LF-OIL bar")  # s-:方形
            ax.plot(date, rf_oil, 's-', color='green', label="RF-OIL bar")  # o-:圆形
            ax.plot(date, lb_oil, 's-', color='blue', label="LB-OIL bar")  # o-:圆形
            ax.plot(date, rb_oil, 's-', color='steelblue',label="RB-OIL bar")  # o-:圆形
            ax.plot(date, f_air, 'o-', color='orange', label="F-AIR bar")  # o-:圆形
            ax.plot(date, b_air, 'o-', color='pink', label="B-AIR bar")  # o-:圆形
            
            
            ax2.plot(date, e_speed, 's-', color='yellow', label="E_SPEED m/s")  # s-:方形
            ax2.plot(date, c_speed, 's-', color='black', label="C_SPEED m/s")  # o-:圆形
            ax.legend(loc="upper left")
            ax2.legend(loc="upper right")
            # ax2.set_ylabel('speed m/s')
            ax.set_ylabel('pressure bar')

    ani = animation.FuncAnimation(
        fig=fig, func=animation_function, frames=1, interval=200)
    plt.show()    

def main(argv):
    global major, minor, xsize, ysize, file, review, offset
    try:
        opts, args = getopt.getopt(
            argv, "hx:a:i:y:f:ro:", ["xsize", "ysize=", "ymajor=", "yminor=", "file=", "review", "offset="])
    except getopt.GetoptError:
        print("front_end.py -x <x_size> -a <ymajor> -i <yminor> -y <y_size> -f <file_name> -r")
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print("front_end.py -x <x_size> -a <ymajor> -i <yminor> y <y_size> -f <file_name> -r")
            sys.exit()
        elif opt in ('-a', '--ymajor'):
            major = float(arg)
            print(major)
        elif opt in ('-i', '--yminor'):
            minor = float(arg)
        elif opt in ('-x', '--xsize'):
            xsize = int(arg)
        elif opt in ('-y', '--ysize'):
            ysize = float(arg)
        elif opt in('-f', '--file'): 
            file = arg
            print(file)
        elif opt in('-r', '--review'):
            review = 1
        elif opt in('-o', '--offset'):
            offset = int(arg)

if __name__ == "__main__":

    main(sys.argv[1:])
    if (review == 0):
        drow()
    else:
        drow2()
