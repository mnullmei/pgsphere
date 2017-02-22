SET client_min_messages = 'notice';

SELECT '29/0-3,7'::smoc;
SELECT '29/0,1,2,3,7'::smoc;


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

