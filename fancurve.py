#!/usr/bin/python3
# A script to generate ASL code to set fan curve for the Clevo IT5570 EC

n = int(input("Enter number of fans: "))
t = []
d = []
t.append(int(input("Enter p1 temp: ")))
d.append(int(input("Enter p1 duty: ")))
t.append(int(input("Enter p2 temp: ")))
d.append(int(input("Enter p2 duty: ")))
t.append(int(input("Enter p3 temp: ")))
d.append(int(input("Enter p3 duty: ")))
t.append(int(input("Enter p4 temp: ")))
d.append(int(input("Enter p4 duty: ")))

for i in range(1, n+1):
    for j in range(0, 4):
        print(f"P{j+1}F{i} = {hex(t[j])}")
        print(f"P{j+1}D{i} = {hex(int(d[j] * 255 / 100))}")
    print()
    for j in range(0, 3):
        slope = int((d[j+1] - d[j]) / (t[j+1] - t[j]) * 2.55 * 16)
        print(f"SH{i}{j+1} = {hex(slope >> 8)}")
        print(f"SL{i}{j+1} = {hex(slope & 0xFF)}")
    print()

