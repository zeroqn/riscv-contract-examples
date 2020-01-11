#[cfg(test)]
mod tests {
    use cita_vm::{Context, FakeVM, InterpreterResult, InterpreterType, Transaction};
    use ethereum_types::{Address, U256};

    use std::{fs, ops::Deref, rc::Rc};

    const BALLOT_BINARY_PATH: &str = "./build/ballot";

    #[derive(Debug, Clone)]
    struct User {
        addr: Address,
        nonce: Rc<usize>,
    }

    impl Deref for User {
        type Target = Address;

        fn deref(&self) -> &Address {
            &self.addr
        }
    }

    impl User {
        pub fn new(addr: Address) -> Self {
            User {
                addr,
                nonce: Rc::new(1),
            }
        }

        pub fn nonce(&mut self) -> U256 {
            let nonce = *self.nonce.as_ref();
            *Rc::get_mut(&mut self.nonce).expect("nonce") = nonce + 1;

            U256::from(nonce)
        }
    }

    struct Ballot {
        vm: FakeVM,
        addr: Address,
    }

    impl Ballot {
        pub fn deploy(vm: FakeVM, owner: &mut User) -> Self {
            let tx = Transaction {
                from: **owner,
                to: None,
                value: U256::from(0),
                nonce: owner.nonce(),
                gas_limit: 1_000_000,
                gas_price: U256::from(1),
                input: fs::read(BALLOT_BINARY_PATH).expect("read contract"),
                itype: InterpreterType::RISCV,
            };

            let addr = match vm.executor.exec(Context::default(), tx).expect("deploy") {
                InterpreterResult::Create(_, _, _, addr) => addr,
                _ => unreachable!(),
            };

            let ballot = Ballot { vm, addr };
            ballot.set_owner(owner);

            ballot
        }

        pub fn get_owner(&self, caller: &mut User) -> Address {
            let tx = self.create_tx(caller, vec!["get_owner".into()]);
            let val = self.exec_unwrap(tx, "get_owner");

            Address::from_slice(&val)
        }

        fn set_owner(&self, caller: &mut User) {
            let tx = self.create_tx(caller, vec!["set_owner".into()]);
            self.exec_unwrap(tx, "set_owner");
        }

        pub fn get_topic(&self, caller: &mut User) -> String {
            let tx = self.create_tx(caller, vec!["get_topic".into()]);
            let val = self.exec_unwrap(tx, "get_topic");

            String::from_utf8(val).expect("parse topic")
        }

        pub fn set_topic(&self, owner: &mut User, topic: &str) {
            let tx = self.create_tx(owner, vec!["set_topic".into(), topic.into()]);
            self.exec_unwrap(tx, "set_topic");
        }

        pub fn is_voter(&self, caller: &mut User) -> bool {
            let tx = self.create_tx(caller, vec!["is_voter".into()]);
            let val = self.exec_unwrap(tx, "is_voter");

            if val.get(0) == Some(&1) {
                true
            } else {
                false
            }
        }

        pub fn authorize_vote_right(&self, owner: &mut User, user: &User) {
            let user_addr = hex::encode(user.to_vec()).as_bytes().to_vec();

            let tx = self.create_tx(owner, vec!["authorize_vote_right".into(), user_addr]);

            self.exec_unwrap(tx, "authorize_vote_right");
        }

        fn create_tx(&self, caller: &mut User, params: Vec<Vec<u8>>) -> Transaction {
            Transaction {
                from: **caller,
                to: Some(self.addr),
                value: U256::from(0),
                nonce: caller.nonce(),
                gas_limit: 1_000_000,
                gas_price: U256::from(1),
                input: cita_vm::riscv::combine_parameters(params),
                itype: cita_vm::InterpreterType::RISCV,
            }
        }

        fn exec_unwrap(&self, tx: Transaction, name: &'static str) -> Vec<u8> {
            match self.vm.executor.exec(Context::default(), tx).expect(name) {
                InterpreterResult::Normal(val, ..) => val,
                _ => panic!(name),
            }
        }
    }

    #[test]
    fn test_ballot() {
        let vm = cita_vm::FakeVM::new();
        let mut owner = User::new(vm.account1);
        let mut voter = User::new(vm.account2);

        let ballot = Ballot::deploy(vm, &mut owner);
        assert_eq!(ballot.get_owner(&mut voter), *owner, "should have owner");

        ballot.set_topic(&mut owner, "Moon ?");
        assert_eq!(&ballot.get_topic(&mut voter), "Moon ?", "should set topic");

        assert_eq!(ballot.is_voter(&mut voter), false, "no right");

        ballot.authorize_vote_right(&mut owner, &voter);
        assert_eq!(ballot.is_voter(&mut voter), true, "should have right");
    }
}
