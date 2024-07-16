-- Define a global string variable
global_string = "global string"

-- Define an empty function
function empty_func()
    -- This function does nothing
end

-- Define a function that returns the length of a string
function strlen_func(s)
	return #s
end

function bubble_sort(a)
    repeat
        local swapped = false
        for i = 2, #a do
            if a[i - 1] > a[i] then
                a[i], a[i - 1] = a[i - 1], a[i]
                swapped = true
            end
        end
    until not swapped
end
