
const ret = setTimeout(
    () => {
        console.log('hello world+++++++');
    return '1111';
    },
    2000
);
console.log("result is ", ret);

clearTimeout(ret);

console.log('end');