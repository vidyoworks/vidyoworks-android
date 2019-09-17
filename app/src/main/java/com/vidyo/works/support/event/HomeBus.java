package com.vidyo.works.support.event;

import com.vidyo.works.support.event.base.BusBase;
import com.vidyo.works.support.event.base.CallBase;

public class HomeBus<T> extends BusBase<T, HomeBus.Call> {

    public HomeBus(Call call, T... value) {
        super(call, value);
    }

    public enum Call implements CallBase {
        INIT_PASS, CALLING, STARTED, ENDED, MESSAGE, ERROR, LOGIN, LOGOUT
    }

    @Override
    public Call getCall() {
        return super.getCall();
    }
}