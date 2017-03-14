# New Quantum opcodes spec

## OP_EXEC

* Inputs:
	* version -- The VM version to use (currently only version 1, for EVM is supported)
	* bytecode -- The bytecode to be executed
* Outputs
	* None, has `OP_RETURN` behavior for version 1, but is distinct in that it is not instantly marked as "unspendable", and thus eligible for trimming
* Special
	* OP_EXEC is only valid within a vout `scriptPubKey`. It can not be executed as part of a redeeming `scriptSig` script
	* OP_EXEC forces a `vout` to be executed when added to the blockchain, rather than when being spent. 

`OP_EXEC` is a new opcode to enable external VM bytecode to be executed. This opcode allows for transactions to use the EVM to create smart contracts. 

## OP_EXEC_ASSIGN

* Inputs
	* Smart contract address (without checksum/version padding)
	* version -- The VM version to use. In (all?) cases, the VM version of the smart contract must match the VM version of an OP_EXEC_ASSIGN
	* bytecode -- The bytecode to use for sending a message to the smart contract. Metadata will be given to the VM with info such as amount of transaction etc.
* Outputs
	* Returns 0 usually. Returns 1 if determined that the input smart contract has spent this vout, with a miner spending this by using an OP_EXEC_SPEND
	* similar to `OP_EXEC`, this will force a vout to be executed when added to the blockchain, rather than when being spent. 

`OP_EXEC_ASSIGN` is the opcode used to send money and messages to a smart contract that has already been published. The transaction is considered
invalid if the smart contract address does not exist and is confirmed by at least 1 block. Attaching info such as a public key should be done by putting it into the VM bytecode

## OP_EXEC_SPEND

* Inputs
	* None
* Outputs
	* None, except for internal state changing
* Special
	* It is SECURITY CRITICAL that this opcode not be allowed to be used in normal transactions, nor distributed on the P2P network. 
	* This can only be used in transactions created by miners, and only if a smart contract spend warrants it. 
	* All nodes must check blocks with these transactions to ensure that smart contracts actually selected for their inputs to be spent
	* This opcode is only valid in redeeming `scriptSig` scripts.
	* This sets special internal state so that OP_EXEC_ASSIGN returns 1 and thus making the transaction spendable

`OP_EXEC_SPEND` is the special opcode which smart contracts implicitly use to spend the money assigned to them. When a smart contract chooses to send money to somewhere else the transaction will look like so:

block 1: vout scriptPubKey with OP_EXEC_ASSIGN
block 2: vin `scriptSig` with OP_EXEC_SPEND

So in this way the total script might look like

	EXEC_SPEND ;inserted from scriptSig
	PUSH [address]
	PUSH 1 ;version
	PUSH [bytecode]
	OP_EXEC_ASSIGN

`EXEC_SPEND` will thus make EXEC_ASSIGN return true, and not actually execute the bytecode. (it was executed when added to the blockchain initially)

Any transaction using this opcode is invalid unless a smart contract using the given [address] has determined (including with deterministic change etc) to spend this vout within the current final block.


