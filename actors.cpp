#include "actors.h"
#include "events.h"
#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/hfunc.h>

static auto ShouldContinue = std::make_shared<TProgramShouldContinue>();


/*
Вам нужно написать реализацию TReadActor, TMaximumPrimeDevisorActor, TWriteActor
*/

/*
Требования к TReadActor:
1. Рекомендуется отнаследовать этот актор от NActors::TActorBootstrapped
2. В Boostrap этот актор отправляет себе NActors::TEvents::TEvWakeup
3. После получения этого сообщения считывается новое int64_t значение из strm
4. После этого порождается новый TMaximumPrimeDevisorActor который занимается вычислениями
5. Далее актор посылает себе сообщение NActors::TEvents::TEvWakeup чтобы не блокировать поток этим актором
6. Актор дожидается завершения всех TMaximumPrimeDevisorActor через TEvents::TEvDone
7. Когда чтение из файла завершено и получены подтверждения от всех TMaximumPrimeDevisorActor,
этот актор отправляет сообщение NActors::TEvents::TEvPoisonPill в TWriteActor

TReadActor
    Bootstrap:
        send(self, NActors::TEvents::TEvWakeup)

    NActors::TEvents::TEvWakeup:
        if read(strm) -> value:
            register(TMaximumPrimeDevisorActor(value, self, receipment))
            send(self, NActors::TEvents::TEvWakeup)
        else:
            ...

    TEvents::TEvDone:
        if Finish:
            send(receipment, NActors::TEvents::TEvPoisonPill)
        else:
            ...
*/

// TODO: напишите реализацию TReadActor
class TReadActor : public NActors::TActorBootstrapped<TReadActor> {
private:
    const NActors::TActorId Writer;

    long MaximumPrimeDevisorActorCount;
    bool Finish;

    void HandleWakeup() {
        long value;
        try {
            Cin >> value;   
        } catch(...) {
            Finish = true;
            if (MaximumPrimeDevisorActorCount == 0)
                Send(Writer, std::make_unique<NActors::TEvents::TEvPoisonPill>()); 
            return;
        }
        
        Register(CreateMaximumPrimeDevisorActor(value, SelfId(), Writer));
        MaximumPrimeDevisorActorCount++;

        Send(SelfId(), std::make_unique<NActors::TEvents::TEvWakeup>());
    }

    void HandleDone() {
        MaximumPrimeDevisorActorCount--;
        if (Finish && MaximumPrimeDevisorActorCount == 0)
            Send(Writer, std::make_unique<NActors::TEvents::TEvPoisonPill>()); 
    }

    // STFUNC(StateFunc) {
    //     switch (ev->GetTypeRewrite()) {
    //         cFunc(NActors::TEvents::TEvWakeup::EventType, HandleWakeup);
    //     default:
    //         break;
    //     }
    // }
    
    STRICT_STFUNC(StateFunc, {
        cFunc(NActors::TEvents::TEvWakeup::EventType, HandleWakeup);
        cFunc(TEvents::TEvDone::EventType, HandleDone);
    });

public:
    TReadActor(NActors::TActorId writer) : Writer(writer) {
        MaximumPrimeDevisorActorCount = 0;
        Finish = false;
    }

    void Bootstrap() {
        Become(&TThis::StateFunc);
        Send(SelfId(), std::make_unique<NActors::TEvents::TEvWakeup>());
    }
};

THolder<NActors::IActor> CreateReadActor(NActors::TActorId writer) {
    return MakeHolder<TReadActor>(writer);
}

/*
Требования к TMaximumPrimeDevisorActor:
1. Рекомендуется отнаследовать этот актор от NActors::TActorBootstrapped
2. В конструкторе этот актор принимает:
 - значение для которого нужно вычислить простое число
 - ActorId отправителя (ReadActor)
 - ActorId получателя (WriteActor)
2. В Boostrap этот актор отправляет себе NActors::TEvents::TEvWakeup по вызову которого происходит вызов Handler для вычислений
3. Вычисления нельзя проводить больше 10 миллисекунд
4. По истечении этого времени нужно сохранить текущее состояние вычислений в акторе и отправить себе NActors::TEvents::TEvWakeup
5. Когда результат вычислен он посылается в TWriteActor c использованием сообщения TEvWriteValueRequest
6. Далее отправляет ReadActor сообщение TEvents::TEvDone
7. Завершает свою работу

TMaximumPrimeDevisorActor
    Bootstrap:
        send(self, NActors::TEvents::TEvWakeup)

    NActors::TEvents::TEvWakeup:
        calculate
        if > 10 ms:
            Send(SelfId(), NActors::TEvents::TEvWakeup)
        else:
            Send(WriteActor, TEvents::TEvWriteValueRequest)
            Send(ReadActor, TEvents::TEvDone)
            PassAway()
*/

// TODO: напишите реализацию TMaximumPrimeDevisorActor

class TMaximumPrimeDevisorActor : public NActors::TActorBootstrapped<TMaximumPrimeDevisorActor> {
private:
    long Value;
    NActors::TActorId Reader;
    NActors::TActorId Writer;


    void HandleWakeup() {
        // calc Max Prime Devisor
        long value  = Value * 10;

        Send(Writer, std::make_unique<TEvents::TEvWriteValueRequest>(value));
        Send(Reader, std::make_unique<TEvents::TEvDone>());
        PassAway();
    }

    // STFUNC(StateFunc) {
    //     switch (ev->GetTypeRewrite()) {
    //         cFunc(NActors::TEvents::TEvWakeup::EventType, HandleWakeup);
    //     default:
    //         break;
    //     }
    // }

    STRICT_STFUNC(StateFunc, {
        cFunc(NActors::TEvents::TEvWakeup::EventType, HandleWakeup);
    });

public:
    TMaximumPrimeDevisorActor(long value, NActors::TActorId reader, NActors::TActorId writer) 
        : Value(value), Reader(reader), Writer(writer) {}

    void Bootstrap() {
        Become(&TThis::StateFunc);
        Send(SelfId(), std::make_unique<NActors::TEvents::TEvWakeup>());
    }
};

NActors::IActor* CreateMaximumPrimeDevisorActor(long& value, NActors::TActorId reader, NActors::TActorId writer) {
    return new TMaximumPrimeDevisorActor(value, reader, writer);
}

/*
Требования к TWriteActor:
1. Рекомендуется отнаследовать этот актор от NActors::TActor
2. Этот актор получает два типа сообщений NActors::TEvents::TEvPoisonPill::EventType и TEvents::TEvWriteValueRequest
2. В случае TEvents::TEvWriteValueRequest он принимает результат посчитанный в TMaximumPrimeDevisorActor и прибавляет его к локальной сумме
4. В случае NActors::TEvents::TEvPoisonPill::EventType актор выводит в Cout посчитанную локальнкую сумму, проставляет ShouldStop и завершает свое выполнение через PassAway

TWriteActor
    TEvents::TEvWriteValueRequest ev:
        Sum += ev->Value

    NActors::TEvents::TEvPoisonPill::EventType:
        Cout << Sum << Endl;
        ShouldStop()
        PassAway()
*/

// TODO: напишите реализацию TWriteActor

class TWriteActor : public NActors::TActor<TWriteActor> {
private:
    long sum;

    void WriteValueHandle(TEvents::TEvWriteValueRequest::TPtr &ev) {
        sum += ev->Get()->value;
    }

    void PoisonPillHandle() {
        Cout << sum << Endl;
        GetProgramShouldContinue()->ShouldStop();
        PassAway();
    }

    // STFUNC(StateFunc) {
    //     switch (ev->GetTypeRewrite()) {
    //         hFunc(TEvents::TEvWriteValueRequest, WriteValueHandle);
    //         cFunc(NActors::TEvents::TEvPoisonPill::EventType, PoisonPillHandle);
    //     default:
    //         break;
    //     }
    // }

    STRICT_STFUNC(StateFunc, {
        hFunc(TEvents::TEvWriteValueRequest, WriteValueHandle);
        cFunc(NActors::TEvents::TEvPoisonPill::EventType, PoisonPillHandle);
    });

public:
    TWriteActor()  : TActor(&TThis::StateFunc), sum(0) {}

};

THolder<NActors::IActor> CreateWriteActor() {
    return MakeHolder<TWriteActor>();
}

///////////////////////////////////////////////////////////////////////////////////


class TSelfPingActor : public NActors::TActorBootstrapped<TSelfPingActor> {
    TDuration Latency;
    TInstant LastTime;

public:
    TSelfPingActor(const TDuration& latency)
        : Latency(latency)
    {}

    void Bootstrap() {
        LastTime = TInstant::Now();
        Become(&TSelfPingActor::StateFunc);
        Send(SelfId(), std::make_unique<NActors::TEvents::TEvWakeup>());
    }

    STRICT_STFUNC(StateFunc, {
        cFunc(NActors::TEvents::TEvWakeup::EventType, HandleWakeup);
    });

    void HandleWakeup() {
        auto now = TInstant::Now();
        TDuration delta = now - LastTime;
        Y_VERIFY(delta <= Latency, "Latency too big");
        LastTime = now;
        Send(SelfId(), std::make_unique<NActors::TEvents::TEvWakeup>());
    }
};


THolder<NActors::IActor> CreateSelfPingActor(const TDuration& latency) {
    return MakeHolder<TSelfPingActor>(latency);
}

std::shared_ptr<TProgramShouldContinue> GetProgramShouldContinue() {
    return ShouldContinue;
}
