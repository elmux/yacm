/**
 * @file   uiViewWork.c
 * @author Toni Baumann (bauma12@bfh.ch)
 * @date   May 23, 2011
 * @brief  Defines the Work view for userInterface.c
 */
#include <stdio.h>
#include <string.h>
#include "defines.h"
#include "userInterface.h"
#include "inputController.h"
#include "ledController.h"
#include "logic.h"
#include "timer.h"

/* how long are we waiting until new button events register */
#define BUTTON_DELAY	800

/* blink interval definition */
#define PRODUCT_BLINK_TIME_ON	250
#define PRODUCT_BLINK_TIME_OFF	250

static TIMER delayTimer;

/**
 * run action of work view
 */
static void run(void) {
	MakeCoffeeProcessInstanceViewModel makingCoffee = getCoffeeMakingProcessInstanceViewModel();
	int activeButton = PRODUCT_1_BUTTON;

	/* wait a bit until we check anew for button events */
	if ((delayTimer == NULL) || (isTimerElapsed(delayTimer))) {
		/*make sure we don't use timer again */
		delayTimer = NULL;

		/* let's get the right button to query for stopping */
		switch ( makingCoffee.productIndex ) {
			case 0: activeButton = PRODUCT_1_BUTTON;
			break;
			case 1: activeButton = PRODUCT_2_BUTTON;
			break;
			case 2: activeButton = PRODUCT_3_BUTTON;
			break;
			case 3: activeButton = PRODUCT_4_BUTTON;
			break;
			default: activeButton = PRODUCT_1_BUTTON;
			break;
		}

		/* user tries to stop making coffee? */
		if (getButtonState(activeButton) == button_on) {
			abortMakingCoffee();
		}
	}

	/* Did someone turn the coffeemaker off? */
	if (getSwitchState(POWER_SWITCH) == switch_off) {
#ifdef DEBUG
		printf("Detected power switch to off\n");
	#endif
		switchOff();
	}

	/* update blinking Leds */
	updateAllLeds();
}

/**
 * update action of work view
 */
static void update(void) {
	CoffeeMakerViewModel *coffeemaker = getNewCoffeeMakerState();

	/* string array for activity messages */
	char *currentActivityText[4] = {
			"Warming up!",
			"Delivering milk!",
			"Delivering coffee!",
			""
	};
	int currentActivityIndex = 3;
	CoffeeMakingActivity currentActivity;
	DisplayState *displaystate = getDisplayState();

	/*Clear activity message */
	GrSetGCForeground(displaystate->gContextID, BLACK);
	GrFillRect(displaystate->gWinID,displaystate->gContextID,120, 60, 150, 20);

	displaystate->gContextID = GrNewGC();
	MakeCoffeeProcessInstanceViewModel activeProduct = getCoffeeMakingProcessInstanceViewModel();

	/* find out what activity message to show on screen */
	currentActivity = activeProduct.currentActivity;
	if (currentActivity == coffeeMakingActivity_warmingUp) {
		currentActivityIndex = 0;
	}
	if (currentActivity == coffeeMakingActivity_deliveringMilk) {
		currentActivityIndex = 1;
	}
	if (currentActivity == coffeeMakingActivity_deliveringCoffee) {
		currentActivityIndex = 2;
	}

	/* Back- Foreground color related stuff */
	GrSetGCForeground(displaystate->gContextID, YELLOW);
	GrSetGCUseBackground(displaystate->gContextID, GR_FALSE);

	/* Select fonts */
	displaystate->font = GrCreateFont((unsigned char *) FONTNAME, 14, NULL);
	GrSetGCFont(displaystate->gContextID, displaystate->font);
	GrText(displaystate->gWinID, displaystate->gContextID, 120, 30, "Making coffee...", -1, GR_TFASCII | GR_TFTOP);

	/* show the current activity */
	GrText(displaystate->gWinID, displaystate->gContextID, 120, 60, currentActivityText[currentActivityIndex], -1, GR_TFASCII | GR_TFTOP);
	GrDestroyFont(displaystate->font);

	/* display active product */
#ifdef DEBUG
	printf("uiViewWork.c: Calling showProduct(%d)\n",activeProduct.productIndex);
#endif
	showProduct(activeProduct.productIndex + 1);

	/* display chosen milk selection for active Product*/
#ifdef DEBUG
	printf("uiViewWork.c: Calling showMilkSelection(%d)\n",activeProduct.withMilk);
#endif
	showMilkSelection(activeProduct.withMilk);

	/* let's check the milk sensor */
	if (coffeemaker->isMilkAvailable == FALSE) {
		/* indicate milk sensor state on display*/
		showMilkSensor(TRUE);
	}
	else {
		showMilkSensor(FALSE);
	}

	/* let's check the coffee sensor */
	if (coffeemaker->isCoffeeAvailable == FALSE) {
		/* indicate coffee sensor state on display*/
		showCoffeeSensor(TRUE);
	}
	else {
		showCoffeeSensor(FALSE);
	}
}


/**
 * activate action of work view
 */
static void activate(void) {
	/*start Timer for button release delay*/
	delayTimer = setUpTimer(BUTTON_DELAY);

	/* start blinking led for product */
	setBlinkingFreq(getActiveProductLedId(), PRODUCT_BLINK_TIME_ON, PRODUCT_BLINK_TIME_OFF);
	updateLed(getActiveProductLedId(), led_blinking);

	/* update display */
	update();
}

/**
 * deactivate action of work view
 */
static void deactivate(void) {
	DisplayState *displaystate = getDisplayState();

	/* Turn off all product Leds */
	updateLed(PRODUCT_1_LED, led_off);
	updateLed(PRODUCT_2_LED, led_off);
	updateLed(PRODUCT_3_LED, led_off);
	updateLed(PRODUCT_4_LED, led_off);

	/*Clear screen*/
	GrClearWindow(displaystate->gWinID,GR_FALSE);
}

/**
 * @copydoc getViewWorkActions
 */
CallViewActions getViewWorkActions(void) {
	CallViewActions retval = { &run, &activate, &deactivate, &update };
	return retval;
}
