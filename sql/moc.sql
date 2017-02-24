SET client_min_messages = 'notice';
-- SET client_min_messages = 'warning';

SELECT '29/0-3,7'::smoc;
SELECT '29/0,1,2,3,7'::smoc;

SELECT '29/3-11,70-88,22-34'::smoc;

SELECT '29/5-11,70-88,2-4'::smoc;

SELECT '29/11-18,22-27,31-35,42-55,62-69,100-111,15-49'::smoc;

SELECT '29/1-3,20-30,7-17'::smoc;

SELECT '29/16-32,10-20'::smoc;

SELECT '29/1-3,11-14,17-21,40-50,9-33'::smoc;

SELECT '29/10-20,16-32'::smoc;

SELECT '29/20-30,64-72,89-93,26-100'::smoc;

SELECT '29/3-11,20-30,64-72,89-93,26-100'::smoc;

SELECT '29/3-11,20-30,64-72,89-93,222-333,26-100'::smoc;

SELECT '29/20-30,64-72,89-93,222-333,26-100'::smoc;

SELECT smoc(-1); -- expected: error
SELECT smoc(15); -- expected: error
SELECT smoc(0);  -- expected: '0/'

SELECT smoc('');            -- expected: error
SELECT smoc('abc');         -- expected: error
SELECT smoc('-1/');         -- expected: error
SELECT smoc('30/');         -- expected: error
SELECT smoc('0/');          -- expected: '0/'
SELECT smoc('0/0-3,7');     -- expected: '0/0-3,7'
SELECT smoc('0/0,1,2,3,7'); -- expected: '0/0-3,7'

SELECT ''::smoc;            -- expected: error
SELECT 'abc'::smoc;         -- expected: error
SELECT '-1/'::smoc;         -- expected: error
SELECT '30/'::smoc;         -- expected: error
SELECT '0/'::smoc;          -- expected: '0/'
SELECT '0/0-3,7'::smoc;     -- expected: '0/0-3,7'
SELECT '0/0,1,2,3,7'::smoc; -- expected: '0/0-3,7'

SELECT smoc('2/0,1,2,3,7 4/17,21-33,111');

SELECT smoc('2/0,1,2,3,7 0/17,21-33,111');

