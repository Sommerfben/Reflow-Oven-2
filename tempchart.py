import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import sys, time, math
import serial
import serial.tools.list_ports
PORT = 'COM5'

xsize=800
try:
 ser.close();
except:
 print();
try:
 ser = serial.Serial(PORT, 9600, timeout=100)
except:
 print ('Serial port %s is not available' % PORT);
 portlist=list(serial.tools.list_ports.comports())
 print('Trying with port %s' % portlist[0][0]);
 ser = serial.Serial(portlist[0][0], 9600, timeout=100)
ser.isOpen()

def data_gen():
    t = data_gen.t
    while True:
        t+=1
        strin = ser.readline();
        a = strin.decode('ascii');
        val = float(a)
        yield t, val

def run(data):
    # update the data
    t,y = data
    if y >= 230:
        line.set_color("red")
    elif y <= 100:
        line.set_color("green")
    else:
        line.set_color("black")
    if t>-1:
        xdata.append(t)
        ydata.append(y)
        if t>xsize: # Scroll to the left.
            ax.set_xlim(t-xsize, t)
        line.set_data(xdata, ydata)

    return line,

def on_close_figure(event):
    sys.exit(0)

data_gen.t = -1
fig = plt.figure()
fig.canvas.mpl_connect('close_event', on_close_figure)
ax = fig.add_subplot(111)
line, = ax.plot([], [], lw=2)

ax.set_ylim(0, 300)
ax.set_xlim(0, xsize)
ax.set_xlabel('Time')
ax.set_ylabel('Temperature')
ax.grid()
xdata, ydata = [], []

# Important: Although blit=True makes graphing faster, we need blit=False to prevent
# spurious lines to appear when resizing the stripchart.
ani = animation.FuncAnimation(fig, run, data_gen, blit=False, interval=100, repeat=False)
plt.show()
