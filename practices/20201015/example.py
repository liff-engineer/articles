import sys


class Item:
    def __init__(self, name, price):
        self.name = name
        self.price = price


items = [Item("1", 1), Item("2", 2), Item("3", 3)]

lowest_price = sys.float_info.max
all_equal = True
last_price = items[0]

for item in items:
    if item.price != last_price:
        all_equal = False
    if item.price < lowest_price:
        lowest_price = item.price

    last_price = item.price

printable_string = ""
if all_equal:
    for item in items:
        printable_string += item.name + str(item.price) + "\n"
else:
    for item in items:
        if item.price == lowest_price:
            printable_string += "**" + item.name + str(item.price) + "**\n"
        else:
            printable_string += item.name + str(item.price) + "\n"

print(printable_string.rstrip("\n"))
