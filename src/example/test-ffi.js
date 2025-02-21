import { CFunction, CCallback } from 'toyjsruntime:jsffi'

//library name, function name, nfixedargs, return value type representation, ...arguments type representation
// If C function arguments length is fixed, nfixedargs must be null, or be the number of fixed arguments.
const test1 = new CFunction('./libtest_lib.so', 'test1', null, 'void');
test1.invoke(); //

const test2 = new CFunction('./libtest_lib.so', 'test2', null, 'double', 'float', 'double', 'string');
const test3 = new CFunction('./libtest_lib.so', 'test3', null, 'void', ['long', 'double', ['int', 'float']]);

console.log(test2.invoke(3.141592654, 2.718281829, 'How do you do?'));

test3.invoke([123456789, 3.141592654, 54321, 2.718281829]);

// Define the test4 CFunction


// Define the callback function matching s2 (*fn)(float, double, const char *)
let test4 = new CFunction('./libtest_lib.so', 'test4', null, 'string', 'pointer');
let cb = new CCallback((a, b, c) => {
    console.log('callback begins');
    console.log('arguments are', a, b, c);
    console.log('callback ends');
    return [1, 2, 3, 4];
}, null, ['long', 'double', ['int', 'float']], 'float', 'double', 'string');
console.log('test4 returns', test4.invoke(cb.cfuncptr));