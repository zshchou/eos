#!/bin/bash

# prevent bc from adding \ at end of large hex values
export BC_LINE_LENGTH=9999

# $1 - error message
error()
{
  (>&2 echo $1)
  killAll
  exit 1
}

verifyErrorCode()
{
  rc=$?
  if [[ $rc != 0 ]]; then
    error "FAILURE - $1 returned error code $rc"
  fi
}

killAll()
{
  programs/launcher/launcher -k 9
  kill -9 $WALLETD_PROC_ID
}

cleanup()
{
  rm -rf tn_data_0
  rm -rf test_wallet_0
  rm -rf test_skeleton
}

# $1 - string that contains "transaction_id": "<trans id>", in it
# result <trans id> stored in TRANS_ID
getTransactionId()
{
  TRANS_ID="$(echo "$1" | awk '/transaction_id/ {print $2}')"
  # remove leading/trailing quotes
  TRANS_ID=${TRANS_ID#\"}
  TRANS_ID=${TRANS_ID%\",}
}

# result stored in HEAD_BLOCK_NUM
getHeadBlockNum()
{
  INFO="$(programs/eosc/eosc get info)"
  verifyErrorCode "eosc get info"
  HEAD_BLOCK_NUM="$(echo "$INFO" | awk '/head_block_num/ {print $2}')"
  # remove trailing coma
  HEAD_BLOCK_NUM=${HEAD_BLOCK_NUM%,}
}

waitForNextBlock()
{
  getHeadBlockNum
  NEXT_BLOCK_NUM=$((HEAD_BLOCK_NUM+1))
  while [ "$HEAD_BLOCK_NUM" -lt "$NEXT_BLOCK_NUM" ]; do
    sleep 0.25
    getHeadBlockNum
  done
}

verifyTransaction()
{
    verifyErrorCode "$2"
    getTransactionId "$1"

    waitForNextBlock
    # verify transaction exists
    TRANS_INFO="$(programs/eosc/eosc --wallet-port 8899 get transaction $TRANS_ID)"
    verifyErrorCode "eosc get transaction trans_id of $TRANS_INFO"
}

verifyTable()
{
    verifyErrorCode "$4"
    count=`echo "$1" | grep "$2" | grep -c "$3"`
    if [ $count == 1 ]; then
      error "FAILURE - $4 failed"
    fi
}

# cleanup from last run
cleanup

INITA_PRV_KEY="5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"
LOG_FILE=eosd_run_test.log

# eosd
programs/launcher/launcher
verifyErrorCode "launcher"
sleep 7
count=`grep -c "generated block" tn_data_0/stderr.txt`
if [[ $count == 0 ]]; then
  error "FAILURE - no blocks produced"
fi

# save starting block number
getHeadBlockNum
START_BLOCK_NUM=$HEAD_BLOCK_NUM

# create 3 keys
KEYS="$(programs/eosc/eosc create key)"
verifyErrorCode "eosc create key"
PRV_KEY1="$(echo "$KEYS" | awk '/Private/ {print $3}')"
PUB_KEY1="$(echo "$KEYS" | awk '/Public/ {print $3}')"
KEYS="$(programs/eosc/eosc create key)"
verifyErrorCode "eosc create key"
PRV_KEY2="$(echo "$KEYS" | awk '/Private/ {print $3}')"
PUB_KEY2="$(echo "$KEYS" | awk '/Public/ {print $3}')"
KEYS="$(programs/eosc/eosc create key)"
if [ -z "$PRV_KEY1" ] || [ -z "$PRV_KEY2" ] || [ -z "$PUB_KEY1" ] || [ -z "$PUB_KEY2" ]; then
  error "FAILURE - create keys"
fi

# walletd
programs/eos-walletd/eos-walletd --data-dir test_wallet_0 --http-server-endpoint=127.0.0.1:8899 > test_walletd_output.log 2>&1 &
verifyErrorCode "eos-walletd"
WALLETD_PROC_ID=$!
sleep 3

# import into a wallet
PASSWORD="$(programs/eosc/eosc --wallet-port 8899 wallet create --name test)"
verifyErrorCode "eosc wallet create"
programs/eosc/eosc --wallet-port 8899 wallet import --name test $PRV_KEY1
verifyErrorCode "eosc wallet import"
programs/eosc/eosc --wallet-port 8899 wallet import --name test $PRV_KEY2
verifyErrorCode "eosc wallet import"
programs/eosc/eosc --wallet-port 8899 wallet import --name test $INITA_PRV_KEY
verifyErrorCode "eosc wallet import"


# wallet list
OUTPUT=$(programs/eosc/eosc --wallet-port 8899 wallet list)
verifyErrorCode "eosc wallet list"
count=`echo $OUTPUT | grep "test" | grep -c "\*"`
if [[ $count == 0 ]]; then
  error "FAILURE - wallet list did not include \*"
fi

# ========================
# Currency contract test
# ========================

# create currency account
ACCOUNT_INFO="$(programs/eosc/eosc --wallet-port 8899 create account inita currency $PUB_KEY1 $PUB_KEY2)"
verifyTransaction "$ACCOUNT_INFO" "eosc create account"

# upload a contract
INFO="$(programs/eosc/eosc --wallet-port 8899 set contract currency contracts/currency/currency.wast contracts/currency/currency.abi)"
verifyTransaction "$INFO" "eosc set contract currency"

# verify currency contract has proper initial balance
INFO="$(programs/eosc/eosc --wallet-port 8899 get table currency currency account)"
verifyTable "$INFO" "balance" "1000000000" "eosc get table currency account"

# push message to currency contract
INFO="$(programs/eosc/eosc --wallet-port 8899 push message currency transfer '{"from":"currency","to":"inita","amount":50}' --scope currency,inita --permission currency@active)"
verifyTransaction "$INFO" "eosc push message currency transfer"

# read current contract balance
INFO="$(programs/eosc/eosc --wallet-port 8899 get table inita currency account)"
verifyTable "$INFO" "balance" "50" "eosc get table currency account"

INFO="$(programs/eosc/eosc --wallet-port 8899 get table currency currency account)"
verifyTable "$INFO" "balance" "999999950" "eosc get table currency account"

# ========================
# Skeleton contract test
# ========================

# copy skeleton folder
INFO="$(tools/eoscpp -n test_skeleton)"
verifyErrorCode "eoscpp -n test_skeleton"

# compile skeleton contract
INFO="$(tools/eoscpp -o test_skeleton/test_skeleton.wast test_skeleton/test_skeleton.cpp)"
verifyErrorCode "eoscpp -o test_skeleton"

# create skeleton account
ACCOUNT_INFO="$(programs/eosc/eosc --wallet-port 8899 create account inita skeleton $PUB_KEY1 $PUB_KEY2)"
verifyTransaction "$ACCOUNT_INFO" "eosc create account"

# upload a contract
INFO="$(programs/eosc/eosc --wallet-port 8899 set contract skeleton test_skeleton/test_skeleton.wast test_skeleton/test_skeleton.abi)"
verifyTransaction "$INFO" "eosc set contract skeleton"

# push message to skeleton contract
INFO="$(programs/eosc/eosc --wallet-port 8899 push message skeleton transfer '{"from":"skeleton","to":"inita","amount":50}' --scope skeleton,inita)"
verifyErrorCode $INFO "eosc push message skeleton transfer"

# ========================
# Simple db contract test
# ========================

# create simpledb account
ACCOUNT_INFO="$(programs/eosc/eosc --wallet-port 8899 create account inita simpledb $PUB_KEY1 $PUB_KEY2)"
verifyTransaction "$ACCOUNT_INFO" "eosc create account"

# upload a contract
INFO="$(programs/eosc/eosc --wallet-port 8899 set contract simpledb contracts/simpledb/simpledb.wast contracts/simpledb/simpledb.abi)"
verifyTransaction "$INFO" "eosc set contract simpledb"

# push message insert1 to simpledb contract
INFO="$(programs/eosc/eosc --wallet-port 8899 push message simpledb insert1 '{"key1":100}' --scope simpledb)"
verifyTransaction "$INFO" "eosc push message simpledb insert1"

# read current contract balance
INFO="$(programs/eosc/eosc --wallet-port 8899 get table simpledb simpledb record1)"
verifyTable "$INFO" "key1" "100" "eosc get table simpledb account"



killAll
cleanup
echo SUCCESS!
