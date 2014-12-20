#!/bin/sh
. /etc/bashrc
rm fifo*
rm -r save
workdir=/home/dev/data/c++/qiaoban/larbin-2.6.3/bin
echo ${workdir}
nohup ${workdir}/larbin -c ${workdir}/conf/bjqb.conf >${workdir}/log/bjqb_`date +'%Y%m%d'`.log 2>&1  &
sleep 60
nohup ${workdir}/larbin -c ${workdir}/conf/gqb.conf >${workdir}/log/gqb_`date +'%Y%m%d'`.log 2>&1  &
sleep 120 
nohup ${workdir}/larbin -c ${workdir}/conf/zgql.conf >${workdir}/log/zgql_`date +'%Y%m%d'`.log 2>&1  &
sleep 300 
nohup ${workdir}/larbin -c ${workdir}/conf/bjfao.conf >${workdir}/log/bjfao_`date +'%Y%m%d'`.log 2>&1  &
sleep 300 
nohup ${workdir}/larbin -c ${workdir}/conf/sdzc.conf >${workdir}/log/sdzc_`date +'%Y%m%d'`.log 2>&1  &
sleep 300 
nohup ${workdir}/larbin -c ${workdir}/conf/zgqw.conf >${workdir}/log/zgqw_`date +'%Y%m%d'`.log 2>&1  &
sleep 300 
nohup ${workdir}/larbin -c ${workdir}/conf/bjqn_finance.conf >${workdir}/log/bjqn_finance_`date +'%Y%m%d'`.log 2>&1  &
sleep 300 
nohup ${workdir}/larbin -c ${workdir}/conf/bjhwxr.conf >${workdir}/log/bjhwxr_`date +'%Y%m%d'`.log 2>&1  &
