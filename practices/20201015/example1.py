import sys


class Item:
    def __init__(self, name, price):
        self.name = name
        self.price = price


class ItemsIterator:
    def __init__(self, items):
        self._items = items
        self._index = 0

    def __iter__(self):
        return self

    def __next__(self):
        if self._index < len(self._items):
            result = self._items[self._index]
            self._index += 1
            return result
        raise StopIteration


class Items:
    def __init__(self):
        self._items = []
        self.lowest_price = sys.float_info.max
        self.all_equal = True
        self.last_price = None

    def add_item(self, item):
        current_price = item.price
        if self.lowest_price > current_price:
            self.lowest_price = current_price
        if self.last_price is not None and self.last_price != current_price:
            self.all_equal = False
        self.last_price = current_price
        self._items.append(item)

    def __len__(self):
        return len(self._items)

    def __getitem(self, key):
        return self._items[key]

    def __setitem(self, key, value):
        self._items[key] = value

    def __delitem(self, key):
        del (self._items[key])

    def __iter__(self):
        return ItemsIterator(self._items)


items = Items()
items.add_item(Item("1", 1))
items.add_item(Item("2", 2))
items.add_item(Item("3", 3))

printable_string = ""
if items.all_equal:
    for item in items:
        printable_string += item.name + str(item.price) + "\n"
else:
    for item in items:
        if item.price == items.lowest_price:
            printable_string += "**" + item.name + str(item.price) + "**\n"
        else:
            printable_string += item.name + str(item.price) + "\n"

print(printable_string.rstrip("\n"))
