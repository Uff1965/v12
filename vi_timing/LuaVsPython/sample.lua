global_string = "global string"

function empty_func()
end

function bubble_sort(aa, cmp)
    cmp = cmp or function(l, r) return l < r end

    local a = {table.unpack(aa)}
    local swapped = true
    while swapped do
        swapped = false
        for i = 2, #a do
            if cmp(a[i], a[i - 1]) then
                a[i], a[i - 1] = a[i - 1], a[i]
                swapped = true
            end
        end
    end
    return a
end
