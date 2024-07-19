global_string = "global string"

def empty_func():
    pass

def bubble_sort(t, cmp = None):
    if not cmp:
#       cmp = lambda l, r: l < r
        cmp = c_ascending
    a = list(t)
    swapped = True
    while swapped:
        swapped = False
        for i in range(1, len(a)):
            if cmp(a[i], a[i - 1]):
                a[i], a[i - 1] = a[i - 1], a[i]
                swapped = True
    return a
