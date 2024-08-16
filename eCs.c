#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_ENTITIES 10000
#define MAX_COMPONENTS 64
#define LOOPS 10

typedef uint32_t Entity;

typedef struct Component 
{
    Entity entity;
    void* data;
    size_t dataSize;
    int typeID;

} Component;

typedef struct 
{
    Component components[MAX_COMPONENTS];
    int count;
} ComponentArray;


typedef struct 
{
    bool active;
} EntityManager;

typedef struct
{
    int requiredComponents[MAX_COMPONENTS];
    int count;
} Filter;

typedef struct 
{
    Entity entities[MAX_ENTITIES];
    int count;
} FilteredEntities;

typedef struct 
{
    void (*callback)(Entity);
    FilteredEntities filteredEntities;
} System;


ComponentArray componentArrays[MAX_ENTITIES];
EntityManager entityManager[MAX_ENTITIES];


Entity create_entity() 
{
    static Entity current_entity = 0;

    if (current_entity >= MAX_ENTITIES) 
    {
        fprintf(stderr, "Max entities reached.\n");
        exit(EXIT_FAILURE);
    }

    entityManager[current_entity].active = true;
    return current_entity++;
}

void destroy_entity(Entity entity) 
{
    if (entity >= 0 && entity < MAX_ENTITIES) 
    {
        entityManager[entity].active = false;
        ComponentArray* array = &componentArrays[entity];

        for (int i = 0; i < array->count; i++) 
        {
            free(array->components[i].data);
        }

        array->count = 0;
    }
}

void* add_component(Entity entity, void* data, size_t dataSize, int typeID)
{
    if (entity < 0 || entity >= MAX_ENTITIES) 
    {
        return NULL;
    }

    if(!entityManager[entity].active)
    {
        return NULL;
    }

    ComponentArray* array = &componentArrays[entity];

    if (array->count >= MAX_COMPONENTS) 
    {
        fprintf(stderr, "Max Components reached!!! (Entity : %d).\n", entity);
        return NULL;
    }

    Component* component = &array->components[array->count++];
    component->entity = entity;
    component->data = malloc(dataSize);

    if(!component->data)
    {
        fprintf(stderr, "Memory could not be allocated for Entity : %d !!! .\n", entity);
        exit(EXIT_FAILURE);
    }

    memcpy(component->data, data, dataSize);
    component->dataSize = dataSize;
    component->typeID = typeID;

    return component->data;
}

void remove_component(Entity entity, int typeID) 
{
    if (entity < 0 || entity >= MAX_ENTITIES) 
    {
        return;
    }

    if(!entityManager[entity].active)
    {
        return;
    }

    ComponentArray* array = &componentArrays[entity];
    
    for (int i = 0; i < array->count; i++) 
    {
        if (array->components[i].typeID == typeID) 
        {
            free(array->components[i].data);
            array->components[i] = array->components[--array->count];
            return;
        }
    }
}

void* get_component(Entity entity, int typeID) 
{
    if (entity < 0 || entity >= MAX_ENTITIES) 
    {
        return NULL;
    }

    if(!entityManager[entity].active)
    {
        return NULL;
    }

    ComponentArray* array = &componentArrays[entity];
    for (int i = 0; i < array->count; i++) 
    {
        if (array->components[i].typeID == typeID) 
        {
            return array->components[i].data;
        }
    }

    return NULL;
}

bool has_component(Entity entity, int typeID) 
{
    if (entity < 0 || entity >= MAX_ENTITIES) 
    {
        return false;
    }

    if(!entityManager[entity].active)
    {
        return false;
    }

    ComponentArray* array = &componentArrays[entity];
    for (int i = 0; i < array->count; i++) 
    {
        if (array->components[i].typeID == typeID) 
        {
            return true;
        }
    }

    return false;
}


bool matches_filter(Entity entity, Filter* filter)
{
    if (entity < 0 || entity >= MAX_ENTITIES) 
    {
        return false;
    }

    if(!entityManager[entity].active)
    {
        return false;
    }

    for (int i = 0; i < filter->count; i++)
    {
        int componentId = filter->requiredComponents[i];
        
        if(!has_component(entity, componentId))
        {
            return false;
        }
    }

    return true;
}

FilteredEntities get_filtered_entities(Filter* filter)
{
    FilteredEntities filteredEntities = { .count = 0 };

    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        if(!matches_filter(i, filter))
        {
            continue;
        }

        filteredEntities.entities[filteredEntities.count++] = i;
    }
    
    return filteredEntities;
}

void run_system(System* system)
{
    for (int i = 0; i < system->filteredEntities.count; i++)
    {
        int entity = system->filteredEntities.entities[i];
        system->callback(entity);
    }
}


typedef struct {
    float x, y;
} PositionComponent;

typedef struct {
    float angle;
} RotationComponent;

typedef struct {
    float x, y, speed;
} VelocityComponent;


const static int positionTypeID = 1; 
const static int rotationTypeID = 2; 
const static int velocityTypeID = 3; 

void print_entity(Entity entity)
{
    printf("Entity : %d \n", entity);
}

void move_entity(Entity entity)
{
    VelocityComponent* velocityComponent = (VelocityComponent*) get_component(entity, velocityTypeID);
    PositionComponent* positionComponent = (PositionComponent*) get_component(entity, positionTypeID);

    if(!velocityComponent || !positionComponent)
    {
        fprintf(stderr, "move_entity System failed to fetch data!!! (Entity : %d).\n", entity);
        return;
    }

    positionComponent->x += velocityComponent->speed * velocityComponent->x;
    positionComponent->y += velocityComponent->speed * velocityComponent->y;

    printf("Entity : %d position : %f ; %f \n", entity, positionComponent->x, positionComponent->y);
}


int main()
{
    Entity e1 = create_entity();
    Entity e2 = create_entity();
    Entity e3 = create_entity();
    Entity e4 = create_entity();

    PositionComponent p1 = {0.0f, 0.0f};
    PositionComponent p2 = {10.0f, 10.0f};

    RotationComponent r1 = {0.0f };

    VelocityComponent v1 = {1.0f, 0.5f, 1.0f};
    VelocityComponent v2 = {1.0f, 1.5f, 3.3f};

    add_component(e1, &p1, sizeof(PositionComponent), positionTypeID);
    add_component(e4, &p1, sizeof(PositionComponent), positionTypeID);
    add_component(e2, &p2, sizeof(PositionComponent), positionTypeID);

    add_component(e1, &r1, sizeof(RotationComponent), rotationTypeID);

    add_component(e1, &v1, sizeof(VelocityComponent), velocityTypeID);
    add_component(e2, &v1, sizeof(VelocityComponent), velocityTypeID);
    add_component(e3, &v2, sizeof(VelocityComponent), velocityTypeID);
    add_component(e4, &v2, sizeof(VelocityComponent), velocityTypeID);

    Filter printFilter =  {{velocityTypeID}, 1};
    FilteredEntities printFilteredEntities = get_filtered_entities(&printFilter);

    Filter moveFilter =  {{positionTypeID, velocityTypeID}, 2};
    FilteredEntities moveFilteredEntities = get_filtered_entities(&moveFilter);
    
    System printEntitySystem = {print_entity, printFilteredEntities};
    System moveEntitySystem = {move_entity, moveFilteredEntities};

    run_system(&printEntitySystem);

    printf("\n------------------\n");

    for (int i = 0; i < LOOPS; i++)
    {
        printf("\nloop no. %d started-------------- \n\n",i);
        run_system(&moveEntitySystem);
        printf("\nloop no. %d  done---------------- \n",i);
    }
    
    destroy_entity(e1);
    destroy_entity(e2);
    destroy_entity(e3);
    destroy_entity(e4);

    return 0;
}