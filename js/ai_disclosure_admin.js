(function ($) {
  Backdrop.behaviors.aiDisclosureFieldsetSummaries = {
    attach: function (context) {
      $('fieldset#edit-ai-disclosure', context).backdropSetSummary(function (context) {
        var selected = $('input[name="ai_disclosure[disclosure_level]"]:checked', context);
        if (selected.length) {
          var label = selected.closest('label').text().split('—')[0].trim();
          return label;
        }
        return Backdrop.t('Not set');
      });
    }
  };
})(jQuery);
