select
  * 
from 
  orders o
  ,lineitem l 
where 
  o.o_orderdate>='1993-10-01'
  and o.o_orderdate<'1994-01-01'
  and l.l_returnflag ='R' 
  and l.l_orderkey = o.o_orderkey;


select c_custkey, n_nationkey from customer, nation where c_nationkey = n_nationkey;
