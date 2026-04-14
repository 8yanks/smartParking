<?php

namespace App\Form;

use App\Entity\ParkingSpot;
use App\Entity\Reservation;
use Doctrine\ORM\EntityRepository;
use Symfony\Bridge\Doctrine\Form\Type\EntityType;
use Symfony\Component\Form\AbstractType;
use Symfony\Component\Form\Extension\Core\Type\DateTimeType;
use Symfony\Component\Form\FormBuilderInterface;
use Symfony\Component\OptionsResolver\OptionsResolver;
use Symfony\Component\Validator\Constraints\GreaterThan;
use Symfony\Component\Validator\Constraints\NotBlank;

class ReservationType extends AbstractType
{
    public function buildForm(FormBuilderInterface $builder, array $options): void
    {
        $builder
            ->add('parkingSpot', EntityType::class, [
                'class' => ParkingSpot::class,
                'choice_label' => 'name',
                'label' => 'Place de parking désirée',
                'query_builder' => function (EntityRepository $er) {
                    return $er->createQueryBuilder('p')
                        ->where('p.isActive = :active')
                        ->setParameter('active', true)
                        ->orderBy('p.name', 'ASC');
                },
                'attr' => ['class' => 'glass-input mb-3']
            ])
            ->add('startTime', DateTimeType::class, [
                'widget' => 'single_text',
                'html5' => false,
                'format' => 'dd/MM/yyyy HH:mm',
                'label' => 'Début de réservation',
                'input' => 'datetime_immutable',
                'constraints' => [
                    new NotBlank(),
                ],
                'attr' => ['class' => 'glass-input mb-3 datetime-picker']
            ])
            ->add('endTime', DateTimeType::class, [
                'widget' => 'single_text',
                'html5' => false,
                'format' => 'dd/MM/yyyy HH:mm',
                'label' => 'Fin de réservation',
                'input' => 'datetime_immutable',
                'constraints' => [
                    new NotBlank(),
                ],
                'attr' => ['class' => 'glass-input mb-3 datetime-picker']
            ])
        ;
    }

    public function configureOptions(OptionsResolver $resolver): void
    {
        $resolver->setDefaults([
            'data_class' => Reservation::class,
        ]);
    }
}
