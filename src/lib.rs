#[cfg(test)]
mod tests {
    use ethereum_types::U256;

    use std::fs;

    const BALLOT_BINARY_PATH: &str = "./build/ballot";

    fn read_le_u64(input: &mut &[u8]) -> u64 {
        use std::convert::TryInto;

        let (int_bytes, rest) = input.split_at(std::mem::size_of::<u64>());
        *input = rest;
        u64::from_le_bytes(int_bytes.try_into().unwrap())
    }

    #[test]
    fn test_ballot() {
        let vm = cita_vm::FakeVM::new();

        // Deploy contract
        let tx = cita_vm::Transaction {
            from: vm.account1,
            to: None,
            value: U256::from(0),
            nonce: U256::from(1),
            gas_limit: 1_000_000,
            gas_price: U256::from(1),
            input: fs::read(BALLOT_BINARY_PATH).unwrap(),
            itype: cita_vm::InterpreterType::RISCV,
        };

        let r = vm
            .executor
            .exec(cita_vm::Context::default(), tx)
            .expect("deploy ballot contract");

        let contract_address = match r {
            cita_vm::InterpreterResult::Create(_, _, _, a) => a,
            _ => unreachable!(),
        };

        println!("{}", contract_address);

        let tx = cita_vm::Transaction {
            from: vm.account1,
            to: Some(contract_address),
            value: U256::from(0),
            nonce: U256::from(2),
            gas_limit: 1_000_000,
            gas_price: U256::from(1),
            input: cita_vm::riscv::combine_parameters(vec!["owner".into()]),
            itype: cita_vm::InterpreterType::RISCV,
        };

        let r = vm
            .executor
            .exec(cita_vm::Context::default(), tx)
            .expect("test address");

        let addr = match r {
            cita_vm::InterpreterResult::Normal(val, ..) => val,
            _ => unreachable!(),
        };

        println!("address {:?}", ethereum_types::Address::from_slice(&addr));
    }
}
