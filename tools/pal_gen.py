with open("nes.pal", "rb") as f:
    data = f.read()

palette = []
i = 0
print("[", end=" ")
while i < len(data):
    color = (data[i] << 16) | (data[i+1] << 8) | data[i+2]
    if (i < len(data) - 3):
        end = ", "
    else:
        end = ""
    print(f"{color:#0{8}x}", end=end)
    palette.append(color)
    i += 3
print(" ]")



