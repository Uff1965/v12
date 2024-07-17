# Define a global string variable
global_string = "global string"

# Define an empty function
def empty_func():
    """This function does nothing."""
    pass

def less(l, r):
    return l < r

def bubble_sort(aa, cmp = less):
    if not cmp:
        cmp = less
    a = list(aa)
    swapped = True
    while swapped:
        swapped = False
        for i in range(1, len(a)):
            if cmp(a[i], a[i - 1]):
                a[i], a[i - 1] = a[i - 1], a[i]
                swapped = True
    return a
