import {readFile } from 'toyjsruntime:fs';

console.log("---start");

// const txt = readFile('./Makefile').then((data) => {
//     console.log(data);
// }
// ).catch((err) => {
//     console.error(err);
// });
// console.log('-------------------', txt);

// const ret = await Promise.resolve('x555xxx');
// console.log(ret);


const text = await readFile('./Makefile');

console.log("end", text);