/**
 * @file   uiViewIdle.c
 * @author Toni Baumann (bauma12@bfh.ch)
 * @date   May 23, 2011
 * @brief  Defines the Idle view for userInterface.c
 */

#include <stdio.h>
#include <string.h>
#include "defines.h"
#include "userInterface.h"
#include "inputController.h"
#include "ledController.h"
#include "logic.h"
#include "timer.h"

/**
 * run action of idle view
 */
static void run(void) {
	CoffeeMakerViewModel *coffeemaker = getCoffeeMakerState();

	/* Did someone turn the coffeemaker off? */
	if (getSwitchState(POWER_SWITCH) == switch_off) {
#ifdef DEBUG
		printf("Detected power switch to off\n");
	#endif
		switchOff();
	}

	/* product got selected? */
	if (getButtonState(PRODUCT_1_BUTTON) == button_on) {
		startMakingCoffee(0);
	}
	if (getButtonState(PRODUCT_2_BUTTON) == button_on) {
		startMakingCoffee(1);
	}
	if (getButtonState(PRODUCT_3_BUTTON) == button_on) {
		startMakingCoffee(2);
	}
	if (getButtonState(PRODUCT_4_BUTTON) == button_on) {
		startMakingCoffee(3);
	}

	/* Did someone use the milk selector? */
	if (getSwitchState(MILK_SWITCH) == switch_off) {
		if (coffeemaker->milkPreselectionState == milkPreselection_on) {
			setMilkPreselection(milkPreselection_off);
#ifdef DEBUG
			printf("uiViewIdle.c: Setting milkPreselection to off\n");
#endif
		}
	}
	if (getSwitchState(MILK_SWITCH) == switch_on) {
		if (coffeemaker->milkPreselectionState == milkPreselection_off) {
			setMilkPreselection(milkPreselection_on);
#ifdef DEBUG
			printf("uiViewIdle.c: Setting milkPreselection to on\n");
#endif
		}
	}
	/* Show the pretty lights */
	updateAllLeds();
}

/**
 * update action of idle view
 */
static void update(void) {
#ifdef DEBUG
	printf("uiViewIdle.c: got an update\n");
#endif
	DisplayState *displaystate = getDisplayState();

	/*Clear screen*/
	GrClearWindow(displaystate->gWinID,GR_FALSE);

	/* Back- Foreground color related stuff */
	GrSetGCForeground(displaystate->gContextID, YELLOW);
	GrSetGCUseBackground(displaystate->gContextID, GR_FALSE);

	/* Select fonts */
	displaystate->font = GrCreateFont((unsigned char *) FONTNAME, 14, NULL);
	GrSetGCFont(displaystate->gContextID, displaystate->font);
	GrText(displaystate->gWinID, displaystate->gContextID, 120, 30, "Product Selection:", -1, GR_TFASCII | GR_TFTOP);
	GrDestroyFont(displaystate->font);
	CoffeeMakerViewModel *coffeemaker = getNewCoffeeMakerState();

	/* How many product do we have? */
	int productNumber = coffeemaker->numberOfProducts;

	/* Our display size only allows MAX_PRODUCTS */
	if (productNumber > MAX_PRODUCTS) productNumber = MAX_PRODUCTS;
	for (int i = 1; i < productNumber + 1;i++) {
		showProduct(i);
	}
	if (coffeemaker->milkPreselectionState == milkPreselection_on) {
		/* indicate milk selection on display*/
		showMilkSelection(TRUE);
	}
	else {
		showMilkSelection(FALSE);
	}

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
 * activate action of idle view
 */
static void activate(void) {
	update();
}

/**
 * deactivate action of idle view
 */
static void deactivate(void) {
	DisplayState *displaystate = getDisplayState();

	/*Clear screen*/
	GrClearWindow(displaystate->gWinID,GR_FALSE);
}

/**
 * @copydoc getViewIdleActions
 */
CallViewActions getViewIdleActions(void) {
	CallViewActions retval = { &run, &activate, &deactivate, &update };
	return retval;
}
