/**
 * @file   logic.c
 * @author Ronny Stauffer (staur3@bfh.ch)
 * @date   May 23, 2011
 * @brief  Contains the business logic.
 *
 * Contains the business logic.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "logic.h"
#include "timer.h"
#include "sensorController.h"
#include "machineController.h"

#define DATA_STRUCTURE_IMPLEMENTATION

// Model observers
static NotifyModelChanged observer;
static void notifyObservers();

// Memory management
static void * newObject(void *initializer, size_t size);
static void deleteObject(void *object);

// Product list helpers
static unsigned int getNumberOfProducts();
//static struct ProductListElement * getProductListElement(unsigned int productIndex);
static struct Product * getProduct(unsigned int productIndex);

// Model initialization
static void setUpProducts();

// State machine
// Events
typedef enum {
	event_switchedOn,
	event_switchedOff,
	event_isInitialized,
	event_productSelected,
	event_productionProcessAborted,
	event_productionProcessIsFinished,
	event_ingredientTankIsEmpty,
	event_none
} Event;

typedef enum {
	coffeeMakingEvent_isWarmedUp,
	coffeeMakingEvent_deliverMilk,
	coffeeMakingEvent_milkDelivered,
	coffeeMakingEvent_deliverCoffee,
	coffeeMakingEvent_coffeeDelivered,
	coffeeMakingEvent_none
} CoffeeMakingEvent;

static unsigned int selectedProductIndex;

static void processEvent(Event event);

static void checkIngredientTankSensors();

// Timers
static TIMER initTimer;
static TIMER warmingUpTimer;
static TIMER deliveringMilkTimer;

/**
 * Notify the model observers of a model change.
 */
static void notifyObservers() {
	if (observer) {
		(*observer)();
	}
}


/**
 * The coffee maker model instance.
 */
static struct CoffeeMaker coffeeMaker = {
		.state = coffeeMaker_off,
		.coffee.isAvailable = TRUE,
		.coffee.emptyTankSensorId = SENSOR_1,
		.milk.isAvailable = TRUE,
		.milk.emptyTankSensorId = SENSOR_2//,
		//.products = &coffeeProductListElement
};

static enum SensorState lastEmptyCoffeeTankSensorState = sensor_unknown;
static enum SensorState lastEmptyMilkTankSensorState = sensor_unknown;

/**
 * Special case value for an undefined product index.
 */
#define UNDEFINED_PRODUCT_INDEX 999

/**
 * Special case value for an undefined product definition.
 */
//static struct ProductViewModel undefinedProduct = {
#define UNDEFINED_PRODUCT (struct ProductViewModel){ \
		.name = "<undefined>" \
}

/**
 * Special case value for an inexistent coffee making process instance.
 */
static struct MakeCoffeeProcessInstanceViewModel inexistentCoffeeMakingProcessInstance = {
		.currentActivity = coffeeMakingActivity_undefined
};



/**
 * Instanciates and initializes a new object.
 */
static void * newObject(void *initializer, size_t size) {
	void *object = malloc(size);
	memcpy(object, initializer, size);

	return object;
}

/**
 * Deletes an object
 */
static void deleteObject(void *object) {
	free(object);
}

/**
 * Gets the number of product definitions.
 */
static unsigned int getNumberOfProducts() {
	unsigned int numberOfProducts = 0;

	struct ProductListElement *productListElement = coffeeMaker.products;
	while (productListElement) {
		numberOfProducts++;

		productListElement = productListElement->next;
	}

	return numberOfProducts;
}

static unsigned int getProductIndex(struct Product *product) {
	unsigned int productIndex = 0;

	struct ProductListElement *productListElement = coffeeMaker.products;
	while (productListElement) {
		if (productListElement->product == product) {
			return productIndex;
		}

		productIndex++;

		productListElement = productListElement->next;
	}

	return UNDEFINED_PRODUCT_INDEX;
}

static struct Product * getProduct(unsigned int productIndex) {
	unsigned int i = 0;

	struct ProductListElement *productListElement = coffeeMaker.products;
	while (productListElement) {
		if (i == productIndex) {
			return productListElement->product;
		}

		i++;

		productListElement = productListElement->next;
	}

	return NULL;
}

/**
 * Sets up product definitions.
 * Preliminary the setup is done hardcoded.
 * In the future definitions could possibly read from a file?
 */
static void setUpProducts() {
	//struct Product coffeeProduct = {
	//		.name = "Coffee"
	//};
	struct Product *coffeeProduct = newObject(&(struct Product) {
		.name = "Coffee"
	}, sizeof(struct Product));
	//struct Product espressoProduct = {
	//		.name = "Espresso"
	//};
	struct Product *espressoProduct = newObject(&(struct Product) {
		.name = "Espresso"
	}, sizeof(struct Product));
	//struct Product ristrettoProduct = {
	//		.name = "Ristretto"
	//};
	struct Product *ristrettoProduct = newObject(&(struct Product) {
		.name = "Ristretto"
	}, sizeof(struct Product));
	//struct ProductListElement ristrettoProductListElement = {
	//		.product = &ristrettoProduct
	//};
	//struct ProductListElement espressoProductListElement = {
	//		.product = &espressoProduct,
	//		.next = &ristrettoProductListElement
	//};
	//struct ProductListElement coffeeProductListElement = {
	//		.product = &coffeeProduct,
	//		.next = &espressoProductListElement
	//};
	struct Product *products[] = {
			coffeeProduct,
			espressoProduct,
			ristrettoProduct
	};
	struct ProductListElement *nextProductListElement = NULL;
	int i;
	for (i = 2; i >= 0; i--) {
		struct ProductListElement *productListElement = newObject(&(struct ProductListElement) {
			.product = products[i],
			.next = nextProductListElement
		}, sizeof(struct ProductListElement));
		nextProductListElement = productListElement;
	}
	coffeeMaker.products = nextProductListElement;
}









int setUpBusinessLogic() {
	setUpProducts();

	return TRUE;
}

int tearDownBusinessLogic() {
	// Delete product definitions
	struct ProductListElement *productListElement = coffeeMaker.products;
	coffeeMaker.products = NULL;
	while (productListElement) {
		struct ProductListElement *next = productListElement->next;

		deleteObject(productListElement->product);
		deleteObject(productListElement);

		productListElement = next;
	}

	return TRUE;
}

void registerModelObserver(NotifyModelChanged pObserver) {
	observer = pObserver;
}



struct CoffeeMakerViewModel getCoffeeMakerViewModel() {
	// Map to view model
	struct CoffeeMakerViewModel coffeeMakerViewModel = {
			.state = coffeeMaker.state,
			.isCoffeeAvailable = coffeeMaker.coffee.isAvailable,
			.isMilkAvailable = coffeeMaker.milk.isAvailable,
			.numberOfProducts = getNumberOfProducts(),
			.milkPreselectionState = coffeeMaker.milkPreselectionState,
			.isMakingCoffee = coffeeMaker.ongoingCoffeeMaking ? TRUE : FALSE
	};

	return coffeeMakerViewModel;
}

struct ProductViewModel getProductViewModel(unsigned int productIndex) {
	//struct ProductListElement *productListElement = getProductListElement(productIndex);
	struct Product *product = getProduct(productIndex);
	if (product) {
		// Map to view model
		struct ProductViewModel productViewModel = {
			//.name = product->name
		};
		strcpy(productViewModel.name, product->name);

		return productViewModel;
	}

	return UNDEFINED_PRODUCT;
}

struct MakeCoffeeProcessInstanceViewModel getCoffeeMakingProcessInstanceViewModel() {
	if (coffeeMaker.ongoingCoffeeMaking) {
		struct MakeCoffeeProcessInstanceViewModel coffeeMakingViewModel = {
				.productIndex =  getProductIndex(coffeeMaker.ongoingCoffeeMaking->product),
				.withMilk = coffeeMaker.ongoingCoffeeMaking->withMilk,
				.currentActivity = coffeeMaker.ongoingCoffeeMaking->currentActivity
		};

		return coffeeMakingViewModel;
	}

	return inexistentCoffeeMakingProcessInstance;
}





void switchOn() {
	//if (!(coffeeMaker.state = coffeeMaker_off)) {
	//	return;
	//}

	//coffeeMaker.state = coffeeMaker_initializing;

	//notifyObservers();

	//initTimer = setUpTimer(2000);
	processEvent(event_switchedOn);
}

void switchOff() {
	//if (!(coffeeMaker.state = coffeeMaker_idle || coffeeMaker.state == coffeeMaker_producing)) {
	//	return;
	//}

	//coffeeMaker.state = coffeeMaker_off;

	//notifyObservers();
	processEvent(event_switchedOff);
}

void setMilkPreselection(enum MilkPreselectionState state) {
	coffeeMaker.milkPreselectionState = state;

	notifyObservers();
}

void startMakingCoffee(unsigned int productIndex) {
	selectedProductIndex = productIndex;

	processEvent(event_productSelected);
}

void abortMakingCoffee() {
	processEvent(event_productionProcessAborted);
}




#ifdef SWITCH_IMPLEMENTATION
void runBusinessLogic() {
	// Check empty ingredient tank sensors

	// Check timers
	if (coffeeMaker.state == coffeeMaker_initializing) {
		if (isTimerElapsed(initTimer)) {
			//coffeeMaker.state = coffeeMaker_idle;

			//notifyObservers();
			processEvent(event_isInitialized);
		}
	}

	// Run possibly ongoing coffee making process instance
}

static void processEvent(Event event) {
	switch (coffeeMaker.state) {
	case coffeeMaker_off:
		switch (event) {
		case event_switchedOn:
			coffeeMaker.state = coffeeMaker_initializing;
			initTimer = setUpTimer(2000);
			notifyObservers();
			break;
		}
		break;
	case coffeeMaker_initializing:
		switch (event) {
		case event_isInitialized:
			coffeeMaker.state = coffeeMaker_idle;
			notifyObservers();
			break;
		}
		break;
	case coffeeMaker_idle:
		switch (event) {
		case event_switchedOff:
			coffeeMaker.state = coffeeMaker_off;
			notifyObservers();
			break;
		}
		break;
	case coffeeMaker_producing:
		switch (event) {
		case event_switchedOff:
			coffeeMaker.state = coffeeMaker_off;
			notifyObservers();
			break;
		}
		break;
	}
}
#endif


#ifdef DATA_STRUCTURE_IMPLEMENTATION
// State machine runtime
typedef int (*StatePrecondition)();
typedef void (*StateAction)();
typedef Event (*DoStateAction)();

typedef struct {
	int stateIndex;
	StatePrecondition precondition;
	StateAction entryAction;
	DoStateAction doAction;
	StateAction exitAction;
} State;

//typedef struct {
//   enum  CoffeeMakerState  nextState;
//   State *state;
//} Transition;

typedef struct {
	int isInitialized;
	unsigned int numberOfEvents;
	State *initialState;
	State *activeState;
	State *transitions[];
} StateMachine;

static void setUpStateMachine(StateMachine *stateMachine);
static void runStateMachine(StateMachine *stateMachine);
static void abortStateMachine(StateMachine *stateMachine);
static Event runState(State *state);
static void processEventInt(StateMachine *stateMachine, Event event);
static void activateState(StateMachine *stateMachine, State *nextState);

static void setUpStateMachine(StateMachine *stateMachine) {
	if (stateMachine->isInitialized) {
		return;
	}

	activateState(stateMachine, stateMachine->initialState);

	stateMachine->isInitialized = TRUE;
}

static void runStateMachine(StateMachine *stateMachine) {
	if (!stateMachine->isInitialized) {
		return;
	}

	// Run active state and process events
	Event event = runState(stateMachine->activeState);
	if (event != event_none) {
		processEventInt(stateMachine, event);
	}
}

static void abortStateMachine(StateMachine *stateMachine) {
	if (!stateMachine->isInitialized) {
		return;
	}

	if (stateMachine->activeState->exitAction) {
		stateMachine->activeState->exitAction();
	}
}

static Event runState(State *state) {
	Event event = event_none;

	// If the state has a 'do' action, then run it
	if (state->doAction) {
		event = state->doAction();
	}

	return event;
}

static void processEventInt(StateMachine *stateMachine, Event event) {
	if (!stateMachine->isInitialized) {
		return;
	}

	// Processing an event means looking up the state machine's next state in the transition table
	State *nextState = stateMachine->transitions[stateMachine->activeState->stateIndex * stateMachine->numberOfEvents + event];
	if (nextState) {
		// If next state either has no precondition
		// or the precondition is true...
		if (!nextState->precondition
			|| nextState->precondition()) {
			// Activate next state
			activateState(stateMachine, nextState);
		}
	}
}

static void activateState(StateMachine *stateMachine, State *nextState) {
	// If a state is currently active and the state has an exit action,
	// then run the state's exit action
	if (stateMachine->activeState) {
		if (stateMachine->activeState->exitAction) {
			stateMachine->activeState->exitAction();
		}
	}
	// Make the next state the currently active state
	stateMachine->activeState = nextState;
	// If the (now currently active) state has an entry action,
	// run the state's entry action
	if (stateMachine->activeState->entryAction) {
		stateMachine->activeState->entryAction();
	}
}
#endif










#ifdef DATA_STRUCTURE_IMPLEMENTATION
// State machines

// Main state machine
static StateMachine coffeeMakingProcessMachine;

// Off state
static void offStateEntryAction() {
	coffeeMaker.state = coffeeMaker_off;

	notifyObservers();
}

static State offState = {
	.stateIndex = coffeeMaker_off,
	.entryAction = offStateEntryAction
};

// Initializing state
static void initializingStateEntryAction() {
	coffeeMaker.state = coffeeMaker_initializing;

	initTimer = setUpTimer(2000);

	notifyObservers();
}

static Event initializingStateDoAction() {
	if (isTimerElapsed(initTimer)) {
		return event_isInitialized;
	}

	return event_none;
}

static State initializingState = {
	.stateIndex = coffeeMaker_initializing,
	.entryAction = initializingStateEntryAction,
	.doAction = initializingStateDoAction
};

// Idle state
static void idleStateEntryAction() {
	coffeeMaker.state = coffeeMaker_idle;

	notifyObservers();
}

static State idleState = {
	.stateIndex = coffeeMaker_idle,
	.entryAction = idleStateEntryAction
};

// Producing state
static int producingStatePrecondition() {
	return coffeeMaker.coffee.isAvailable
		&& (coffeeMaker.milkPreselectionState != milkPreselection_on || coffeeMaker.milk.isAvailable);
}

static void startMakeCoffeeProcess(unsigned int productIndex) {
	coffeeMaker.ongoingCoffeeMaking = newObject(&(struct MakeCoffeeProcessInstance) {
		.product = getProduct(productIndex),
		.withMilk = coffeeMaker.milkPreselectionState = milkPreselection_on ? TRUE : FALSE
	}, sizeof(struct MakeCoffeeProcessInstance));

	coffeeMaker.state = coffeeMaker_producing;

	setUpStateMachine(&coffeeMakingProcessMachine);
}

static void producingStateEntryAction() {
	startMakeCoffeeProcess(selectedProductIndex);

	notifyObservers();
}

static Event producingStateDoAction() {
	runStateMachine(&coffeeMakingProcessMachine);

	// Check coffee making process instance progress
	if (coffeeMaker.ongoingCoffeeMaking->currentActivity == coffeeMakingActivity_finished) {
		return event_productionProcessIsFinished;
	}

	return event_none;
}

static void abortMakeCoffeeProcessInstance() {
	if (coffeeMaker.ongoingCoffeeMaking) {
		abortStateMachine(&coffeeMakingProcessMachine);

		deleteObject(coffeeMaker.ongoingCoffeeMaking);
	}
}

static void producingStateExitAction() {
	abortMakeCoffeeProcessInstance();
}

static State producingState = {
	.stateIndex = coffeeMaker_producing,
	.precondition = producingStatePrecondition,
	.entryAction = producingStateEntryAction,
	.doAction = producingStateDoAction,
	.exitAction = producingStateExitAction
};

static StateMachine stateMachine = {
	.numberOfEvents = 7,
	.initialState = &offState,
//static State * stateTransitions[][event_none] = {
	.transitions = {
		/* coffeeMaker_off: */
			/* event_switchedOn: */ &initializingState,
			/* event_switchedOff: */ NULL,
			/* event_isInitialized: */ NULL,
			/* event_productSelected: */ NULL,
			/* event_productionProcessAborted: */ NULL,
			/* event_productionProcessIsFinished: */ NULL,
			/* event_ingredientTankIsEmpty: */ NULL,
		/* coffeeMaker_initializing: */
			/* event_switchedOn: */ NULL,
			/* event_switchedOff: */ NULL,
			/* event_isInitialized: */ &idleState,
			/* event_productSelected: */ NULL,
			/* event_productionProcessAborted: */ NULL,
			/* event_productionProcessIsFinished: */ NULL,
			/* event_ingredientTankIsEmpty: */ NULL,
		/* coffeeMaker_idle: */
			/* event_switchedOn: */ NULL,
			/* event_switchedOff: */ &offState,
			/* event_isInitialized: */ NULL,
			/* event_productSelected: */ &producingState,
			/* event_productionProcessAborted: */ NULL,
			/* event_productionProcessIsFinished: */ NULL,
			/* event_ingredientTankIsEmpty: */ NULL,
		/* coffeeMaker_producing: */
			/* event_switchedOn: */ NULL,
			/* event_switchedOff: */ &offState,
			/* event_isInitialized: */ NULL,
			/* event_productSelected: */ NULL,
			/* event_productionProcessAborted: */ &idleState,
			/* event_productionProcessIsFinished: */ &idleState,
			/* event_ingredientTankIsEmpty: */ &idleState
		}
};

// Make coffee process

// Warming Up activity
static void warmingUpActivityEntryAction() {
	coffeeMaker.ongoingCoffeeMaking->currentActivity = coffeeMakingActivity_warmingUp;

	warmingUpTimer = setUpTimer(1000);

	notifyObservers();
}

static Event warmingUpActivityDoAction() {
	if (isTimerElapsed(warmingUpTimer)) {
		return coffeeMakingEvent_isWarmedUp;
	}

	return coffeeMakingEvent_none;
}

static State warmingUpActivity = {
	.stateIndex = coffeeMakingActivity_warmingUp,
	.entryAction = warmingUpActivityEntryAction,
	.doAction = warmingUpActivityDoAction
};

// With Milk gateway
static Event withMilkGatewayDoAction() {
	if (coffeeMaker.ongoingCoffeeMaking->withMilk) {
		return coffeeMakingEvent_deliverMilk;
	} else {
		return coffeeMakingEvent_deliverCoffee;
	}

	return coffeeMakingEvent_none;
}

static State withMilkGateway = {
	.stateIndex = coffeeMakingActivity_withMilkGateway,
	.doAction = withMilkGatewayDoAction
};

// Delivering Milk activity
static void deliveringMilkActivityEntryAction() {
	coffeeMaker.ongoingCoffeeMaking->currentActivity = coffeeMakingActivity_deliveringMilk;

	startMachine();

	deliveringMilkTimer = setUpTimer(2000);

	notifyObservers();
}

static Event deliveringMilkActivityDoAction() {
	if (isTimerElapsed(deliveringMilkTimer)) {
		return coffeeMakingEvent_milkDelivered;
	}

	return coffeeMakingEvent_none;
}

static void deliveringMilkActivityExitAction() {
	stopMachine();
}

static State deliveringMilkActivity = {
	.stateIndex = coffeeMakingActivity_deliveringMilk,
	.entryAction = deliveringMilkActivityEntryAction,
	.doAction = deliveringMilkActivityDoAction,
	.exitAction = deliveringMilkActivityExitAction
};

// Delivering Coffe activity
static void deliveringCoffeeActivityEntryAction() {
	coffeeMaker.ongoingCoffeeMaking->currentActivity = coffeeMakingActivity_deliveringCoffee;

	startMachine();

	notifyObservers();
}

static Event deliveringCoffeeActivityDoAction() {
	if (!machineRunning()) {
		return coffeeMakingEvent_coffeeDelivered;
	}

	return coffeeMakingEvent_none;
}

static State deliveringCoffeeActivity = {
	.stateIndex = coffeeMakingActivity_deliveringCoffee,
	.entryAction = deliveringCoffeeActivityEntryAction,
	.doAction = deliveringCoffeeActivityDoAction
};

// Finishing activity
static void finishingActivityEntryAction() {
	coffeeMaker.ongoingCoffeeMaking->currentActivity = coffeeMakingActivity_finished;

	notifyObservers();
}

static State finishingActivity = {
	.stateIndex = coffeeMakingActivity_finished,
	.entryAction = finishingActivityEntryAction
};

static StateMachine coffeeMakingProcessMachine = {
	.numberOfEvents = 5,
	.initialState = &warmingUpActivity,
//static State * coffeeMakingActivityTransitions[][coffeeMakingEvent_none] = {
	.transitions = {
		/* coffeeMakingActivity_warmingUp: */
			/* coffeeMakingEvent_isWarmedUp: */ &withMilkGateway,
			/* coffeeMakingEvent_deliverMilk: */ NULL,
			/* coffeeMakingEvent_milkDelivered: */ NULL,
			/* coffeeMakingEvent_deliverCoffee: */ NULL,
			/* coffeeMakingEvent_coffeeDelivered: */ NULL,
		/* coffeeMakingActivity_withMilkGateway: */
			/* coffeeMakingEvent_isWarmedUp: */ NULL,
			/* coffeeMakingEvent_deliverMilk: */ &deliveringMilkActivity,
			/* coffeeMakingEvent_milkDelivered: */ NULL,
			/* coffeeMakingEvent_deliverCoffee: */ &deliveringCoffeeActivity,
			/* coffeeMakingEvent_coffeeDelivered: */ NULL,
		/* coffeeMakingActivity_deliveringMilk: */
			/* coffeeMakingEvent_isWarmedUp: */ NULL,
			/* coffeeMakingEvent_deliverMilk: */ NULL,
			/* coffeeMakingEvent_milkDelivered: */ &deliveringCoffeeActivity,
			/* coffeeMakingEvent_deliverCoffee: */ NULL,
			/* coffeeMakingEvent_coffeeDelivered: */ NULL,
		/* coffeeMakingActivity_deliveringCoffee: */
			/* coffeeMakingEvent_isWarmedUp: */ NULL,
			/* coffeeMakingEvent_deliverMilk: */ NULL,
			/* coffeeMakingEvent_milkDelivered: */ NULL,
			/* coffeeMakingEvent_deliverCoffee: */ NULL,
			/* coffeeMakingEvent_coffeeDelivered: */ &finishingActivity
		}
};




void runBusinessLogic() {
	if (!stateMachine.isInitialized) {
		setUpStateMachine(&stateMachine);
	}

	// Check ingredient tank sensors
	checkIngredientTankSensors();

	// Run state machine
	runStateMachine(&stateMachine);
}

static void processEvent(Event event) {
	processEventInt(&stateMachine, event);
}
#endif




static void checkIngredientTankSensors() {
	// Coffee sensor
	enum SensorState emptyCoffeeTankSensorState = getSensorState(coffeeMaker.coffee.emptyTankSensorId);
	// If sensor state has changed...
	if (emptyCoffeeTankSensorState != lastEmptyCoffeeTankSensorState) {
		// Update model
		coffeeMaker.coffee.isAvailable = !(emptyCoffeeTankSensorState == sensor_alert);

		notifyObservers();

		// Fire event
		if (emptyCoffeeTankSensorState == sensor_alert) {
			processEvent(event_ingredientTankIsEmpty);
		}

		lastEmptyCoffeeTankSensorState = emptyCoffeeTankSensorState;
	}

	// Milk sensor
	enum SensorState emptyMilkTankSensorState = getSensorState(coffeeMaker.milk.emptyTankSensorId);
	// If sensor state has changed...
	if (emptyMilkTankSensorState != lastEmptyMilkTankSensorState) {
		// Update model
		coffeeMaker.milk.isAvailable = !(emptyMilkTankSensorState == sensor_alert);

		notifyObservers();

		// Fire event
		if (emptyMilkTankSensorState == sensor_alert) {
			processEvent(event_ingredientTankIsEmpty);
		}

		lastEmptyMilkTankSensorState = emptyMilkTankSensorState;
	}
}
