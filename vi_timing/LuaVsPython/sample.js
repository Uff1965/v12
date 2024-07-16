// Define a global string variable
let global_string = "global string";

// Define an empty function
function empty_func() {
    // This function does nothing
}

// Define a function that returns the length of a string
function strlen_func(s) {
    return s.length;
}

function func(lst) {
    return lst;
}

function bubbleSort(arr) {
    let n = arr.length;
    for (let i = 0; i < n - 1; i++) {
        for (let j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                [arr[j], arr[j + 1]] = [arr[j + 1], arr[j]];
            }
        }
    }
    return arr;
}

